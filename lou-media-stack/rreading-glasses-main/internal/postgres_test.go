package internal

import (
	"bytes"
	"context"
	"fmt"
	"io"
	"math/rand/v2"
	"strings"
	"sync"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"gopkg.in/loremipsum.v1"
)

func TestPostgres(t *testing.T) {
	ctx := context.Background()

	cache, err := newPostgres(ctx, "postgres://postgres@localhost:5432/test")
	require.NoError(t, err)

	missing, ok := cache.Get(ctx, "missing")
	assert.False(t, ok)
	assert.Nil(t, missing)

	cache.Set(ctx, "expired", []byte{1}, 0)
	expired, ok := cache.Get(ctx, "expired")
	assert.True(t, ok)
	assert.Equal(t, []byte{1}, expired)

	cache.Set(ctx, "cached", []byte{2}, time.Hour)
	cached, ttl, ok := cache.GetWithTTL(ctx, "cached")
	assert.True(t, ok)
	assert.Equal(t, []byte{2}, cached)
	assert.Greater(t, ttl, time.Minute)

	cache.Set(ctx, "cached", []byte{3}, time.Hour)
	updated, ok := cache.Get(ctx, "cached")
	assert.True(t, ok)
	assert.Equal(t, []byte{3}, updated)

	assert.NoError(t, cache.Expire(ctx, "cached"))
}

// TestPostgresCache randomly writes and reads values from the cache
// concurrently to confirm things like our buffer pooling work correctly under
// load.
func TestPostgresCache(t *testing.T) {
	t.Parallel()

	dsn := "postgres://postgres@localhost:5432/test"
	ctx := t.Context()

	cache, err := NewCache(ctx, dsn, nil)
	require.NoError(t, err)

	n := 400
	wg := sync.WaitGroup{}

	for i := range n {
		wg.Add(1)
		go func() {
			defer wg.Done()

			s := strings.Repeat(fmt.Sprint(i), i)
			sleep := time.Duration(rand.Float64() / 10.0 * float64(time.Second))
			time.Sleep(sleep)
			cache.Set(ctx, fmt.Sprint(i), []byte(s), time.Minute)
		}()
	}
	wg.Wait()

	checkCache := func(cache *LayeredCache) {
		for i := range n {
			wg.Add(1)
			go func() {
				defer wg.Done()

				sleep := time.Duration(rand.Float64() / 10.0 * float64(time.Second))
				time.Sleep(sleep)
				actual, ok := cache.Get(ctx, fmt.Sprint(i))
				if i == 0 {
					// Empty value isn't set.
					require.False(t, ok)
					return
				}
				require.True(t, ok)
				expected := strings.Repeat(fmt.Sprint(i), i)
				assert.Equal(t, expected, string(actual))
			}()

		}
		wg.Wait()
	}

	t.Run("warm in-memory cache", func(t *testing.T) {
		t.Parallel()
		checkCache(cache)
	})

	t.Run("cold in-memory cache", func(t *testing.T) {
		t.Parallel()
		// Create a new cache.
		coldCache, err := NewCache(ctx, dsn, nil)
		require.NoError(t, err)
		checkCache(coldCache)
	})

	t.Cleanup(func() {
		for i := range n {
			_ = cache.Expire(ctx, fmt.Sprint(i))
		}
	})
}

func TestStaleData(t *testing.T) {
	// Expired keys should still return data.

	dsn := "postgres://postgres@localhost:5432/test"
	ctx := t.Context()

	cache, err := NewCache(ctx, dsn, nil)
	require.NoError(t, err)

	cache.Set(t.Context(), "KEY", []byte{1}, time.Nanosecond)
	time.Sleep(time.Microsecond)

	bytes, ttl, ok := cache.GetWithTTL(t.Context(), "KEY")
	assert.True(t, ok)
	assert.Equal(t, time.Duration(0), ttl)
	assert.Len(t, bytes, 1)
}

func BenchmarkCompressDecompress(b *testing.B) {
	b.ReportAllocs()

	in := _buffers.Get()
	out := _buffers.Get()

	l := loremipsum.NewWithSeed(42)
	lorum := l.Paragraphs(1000)

	r := bytes.NewReader([]byte(lorum))

	for b.Loop() {
		in.Reset()
		out.Reset()
		_, _ = r.Seek(0, io.SeekStart)

		_ = compress(r, in)
		_ = decompress(b.Context(), bytes.NewReader(in.Bytes()), out)
	}

	assert.Equal(b, lorum, out.String())
}
