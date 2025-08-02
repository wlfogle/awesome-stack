#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <QTabWidget>
#include <QTabBar>
#include <QMenu>
#include <QAction>

class TerminalWidget;

class TabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit TabWidget(QWidget *parent = nullptr);

    void addNewTab(const QString &title = QString(), const QString &workingDir = QString());
    void closeCurrentTab();
    void closeTab(int index);
    void executeCommand(const QString &command);
    
    TerminalWidget* currentTerminal() const;
    TerminalWidget* terminalAt(int index) const;

signals:
    void tabClosed(int index);
    void lastTabClosed();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void onTabCloseRequested(int index);
    void onCurrentChanged(int index);
    void renameCurrentTab();
    void duplicateCurrentTab();
    void closeOtherTabs();
    void closeTabsToTheRight();

private:
    void setupTabBar();
    void createContextMenu();
    
    QMenu *m_contextMenu;
    QAction *m_renameTabAction;
    QAction *m_duplicateTabAction;
    QAction *m_closeTabAction;
    QAction *m_closeOtherTabsAction;
    QAction *m_closeTabsToTheRightAction;
    
    int m_tabCounter;
};

#endif // TABWIDGET_H
