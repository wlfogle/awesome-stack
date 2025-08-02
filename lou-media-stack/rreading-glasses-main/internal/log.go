package internal

import (
	"bytes"
	"context"
	"fmt"
	"log/slog"
	"net/http"
	"os"
	"time"

	"github.com/charmbracelet/lipgloss"
	charm "github.com/charmbracelet/log"
	"github.com/go-chi/chi/v5/middleware"
	"github.com/mattn/go-isatty"
)

var _logHandler *charm.Logger

// Log returns a logger scoped to the request ID if present in the context.
func Log(ctx context.Context) *slog.Logger {
	return slog.Default().With("trace", ctx.Value(middleware.RequestIDKey))
}

// SetLogLevel sets the log level.
func SetLogLevel(l charm.Level) {
	_logHandler.SetLevel(l)
}

// Requestlogger logs some info about requests we handled.
type Requestlogger struct{}

// Wrap applies middleware.
func (Requestlogger) Wrap(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		ctx := r.Context()

		attrs := []slog.Attr{
			slog.String("method", r.Method),
			slog.String("path", r.URL.Path),
			slog.String("ip", r.RemoteAddr),
		}

		start := time.Now()
		ww := middleware.NewWrapResponseWriter(w, r.ProtoMajor)

		body := &bytes.Buffer{}
		ww.Tee(body)

		defer func() {
			status := ww.Status()
			duration := time.Since(start)

			attrs = append([]slog.Attr{
				slog.Int("status", status),
				slog.String("duration", duration.String()),
				slog.Int("bytes", ww.BytesWritten()),
			}, attrs...)

			level := slog.LevelInfo
			switch {
			case status >= 500:
				level = slog.LevelError
				attrs = append(attrs, slog.String("err", body.String()))
			case status >= 400 && status != http.StatusNotFound && status != http.StatusBadRequest:
				level = slog.LevelWarn
			default:
			}

			Log(ctx).LogAttrs(ctx, level,
				fmt.Sprintf("%s %s => HTTP %d (%v)", r.Method, r.URL.String(), ww.Status(), duration),
				attrs...)
		}()

		next.ServeHTTP(ww, r.WithContext(ctx))
	})
}

// set up our default log handler and formatting.
func init() {
	styles := charm.DefaultStyles()
	styles.Keys["err"] = lipgloss.NewStyle().Foreground(lipgloss.Color("204")).Bold(true)
	styles.Keys["status"] = lipgloss.NewStyle().Foreground(lipgloss.Color("86"))
	styles.Values["trace"] = lipgloss.NewStyle().Faint(true)

	_logHandler = charm.NewWithOptions(os.Stdout, charm.Options{
		ReportTimestamp: true,
		TimeFormat:      time.StampMilli,
		Level:           charm.InfoLevel,
	})
	_logHandler.SetStyles(styles)

	// Output JSON in containers.
	if !isatty.IsTerminal(os.Stdout.Fd()) {
		_logHandler.SetFormatter(
			charm.JSONFormatter,
		)
		_logHandler.SetTimeFormat(time.RFC3339)
	}

	logger := slog.New(_logHandler)
	slog.SetDefault(logger)
}
