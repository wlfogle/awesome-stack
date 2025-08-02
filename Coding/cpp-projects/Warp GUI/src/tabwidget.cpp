#include "tabwidget.h"
#include "terminalwidget.h"
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QDir>

TabWidget::TabWidget(QWidget *parent)
    : QTabWidget(parent)
    , m_contextMenu(nullptr)
    , m_tabCounter(0)
{
    setupTabBar();
    createContextMenu();
    
    connect(this, &QTabWidget::tabCloseRequested, this, &TabWidget::onTabCloseRequested);
    connect(this, &QTabWidget::currentChanged, this, &TabWidget::onCurrentChanged);
}

void TabWidget::setupTabBar()
{
    setTabsClosable(true);
    setMovable(true);
    setDocumentMode(true);
    setUsesScrollButtons(true);
    
    // Style the tab bar
    tabBar()->setStyleSheet(
        "QTabBar::tab {"
        "    background: #3c3c3c;"
        "    color: white;"
        "    padding: 8px 16px;"
        "    margin-right: 2px;"
        "    border-top-left-radius: 4px;"
        "    border-top-right-radius: 4px;"
        "}"
        "QTabBar::tab:selected {"
        "    background: #2a82da;"
        "}"
        "QTabBar::tab:hover {"
        "    background: #4a4a4a;"
        "}"
        "QTabBar::close-button {"
        "    image: url(:/icons/close-tab.png);"
        "    subcontrol-position: right;"
        "}"
        "QTabBar::close-button:hover {"
        "    background: rgba(255, 255, 255, 0.2);"
        "    border-radius: 2px;"
        "}"
    );
}

void TabWidget::createContextMenu()
{
    m_contextMenu = new QMenu(this);
    
    m_renameTabAction = m_contextMenu->addAction("Rename Tab");
    m_duplicateTabAction = m_contextMenu->addAction("Duplicate Tab");
    m_contextMenu->addSeparator();
    m_closeTabAction = m_contextMenu->addAction("Close Tab");
    m_closeOtherTabsAction = m_contextMenu->addAction("Close Other Tabs");
    m_closeTabsToTheRightAction = m_contextMenu->addAction("Close Tabs to the Right");
    
    connect(m_renameTabAction, &QAction::triggered, this, &TabWidget::renameCurrentTab);
    connect(m_duplicateTabAction, &QAction::triggered, this, &TabWidget::duplicateCurrentTab);
    connect(m_closeTabAction, &QAction::triggered, this, &TabWidget::closeCurrentTab);
    connect(m_closeOtherTabsAction, &QAction::triggered, this, &TabWidget::closeOtherTabs);
    connect(m_closeTabsToTheRightAction, &QAction::triggered, this, &TabWidget::closeTabsToTheRight);
}

void TabWidget::addNewTab(const QString &title, const QString &workingDir)
{
    QString tabTitle = title.isEmpty() ? QString("Terminal %1").arg(++m_tabCounter) : title;
    QString dir = workingDir.isEmpty() ? QDir::currentPath() : workingDir;
    
    TerminalWidget *terminal = new TerminalWidget(dir, this);
    int index = addTab(terminal, tabTitle);
    setCurrentIndex(index);
    
    // Connect terminal signals
    connect(terminal, &TerminalWidget::titleChanged, [this, terminal](const QString &newTitle) {
        int index = indexOf(terminal);
        if (index != -1) {
            setTabText(index, newTitle);
        }
    });
}

void TabWidget::closeCurrentTab()
{
    closeTab(currentIndex());
}

void TabWidget::closeTab(int index)
{
    if (index < 0 || index >= count()) {
        return;
    }
    
    QWidget *widget = this->widget(index);
    removeTab(index);
    
    if (widget) {
        widget->deleteLater();
    }
    
    emit tabClosed(index);
    
    if (count() == 0) {
        emit lastTabClosed();
    }
}

void TabWidget::executeCommand(const QString &command)
{
    TerminalWidget *terminal = currentTerminal();
    if (terminal) {
        terminal->executeCommand(command);
    }
}

TerminalWidget* TabWidget::currentTerminal() const
{
    return qobject_cast<TerminalWidget*>(currentWidget());
}

TerminalWidget* TabWidget::terminalAt(int index) const
{
    return qobject_cast<TerminalWidget*>(widget(index));
}

void TabWidget::contextMenuEvent(QContextMenuEvent *event)
{
    int tabIndex = tabBar()->tabAt(event->pos());
    if (tabIndex != -1) {
        setCurrentIndex(tabIndex);
        
        // Update context menu actions based on tab count and position
        m_closeOtherTabsAction->setEnabled(count() > 1);
        m_closeTabsToTheRightAction->setEnabled(tabIndex < count() - 1);
        
        m_contextMenu->exec(event->globalPos());
    }
}

void TabWidget::onTabCloseRequested(int index)
{
    closeTab(index);
}

void TabWidget::onCurrentChanged(int index)
{
    Q_UNUSED(index)
    // Update focus to the current terminal
    TerminalWidget *terminal = currentTerminal();
    if (terminal) {
        terminal->setFocus();
    }
}

void TabWidget::renameCurrentTab()
{
    int index = currentIndex();
    if (index == -1) return;
    
    bool ok;
    QString currentTitle = tabText(index);
    QString newTitle = QInputDialog::getText(this, "Rename Tab", 
                                           "Enter new tab name:", 
                                           QLineEdit::Normal, 
                                           currentTitle, &ok);
    
    if (ok && !newTitle.isEmpty()) {
        setTabText(index, newTitle);
    }
}

void TabWidget::duplicateCurrentTab()
{
    TerminalWidget *current = currentTerminal();
    if (current) {
        QString workingDir = current->workingDirectory();
        QString title = tabText(currentIndex()) + " (Copy)";
        addNewTab(title, workingDir);
    }
}

void TabWidget::closeOtherTabs()
{
    int current = currentIndex();
    if (current == -1) return;
    
    // Close tabs after current (in reverse order to maintain indices)
    for (int i = count() - 1; i > current; --i) {
        closeTab(i);
    }
    
    // Close tabs before current (in reverse order)
    for (int i = current - 1; i >= 0; --i) {
        closeTab(i);
    }
}

void TabWidget::closeTabsToTheRight()
{
    int current = currentIndex();
    if (current == -1) return;
    
    // Close tabs after current (in reverse order to maintain indices)
    for (int i = count() - 1; i > current; --i) {
        closeTab(i);
    }
}
