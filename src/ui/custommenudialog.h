#ifndef CUSTOMMENUWIDGET_H
#define CUSTOMMENUWIDGET_H

#include "basestyledwidget.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QTimer>

class SystemStateModel;

class CustomMenuWidget : public BaseStyledWidget
{
    Q_OBJECT

public:
    // Constructor with optional title and description
    explicit CustomMenuWidget(const QStringList &options,
                              SystemStateModel *stateModel,
                              QWidget *parent = nullptr,
                              const QString &title = QString(),
                              const QString &description = QString());

    // Navigation methods (keeping existing names)
    void moveSelectionUp();
    void moveSelectionDown();
    void selectCurrentItem();
    QString currentItemText() const;

    // Professional enhancements
    void setMenuTitle(const QString &title);
    void setMenuDescription(const QString &description);
    void setMenuIcon(const QPixmap &icon);
    void setAutoCloseTimeout(int milliseconds = 0); // 0 = disabled
    void setNavigationHints(bool enabled = true);

    // Menu state management
    bool isValidSelection() const;
    int getCurrentIndex() const;
    int getMenuItemCount() const;
    void setMenuEnabled(bool enabled);

    // Visual feedback
    void highlightCurrentItem();

    void setCurrentSelection(int index);
signals:
    void optionSelected(const QString &option);
    void menuClosed();
    void currentItemChanged(const QString &itemText);
    void menuAboutToClose();
    void selectionChanged(int index, const QString &text);

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onAutoCloseTimeout();
    void onListItemChanged();

private:
    // Core setup methods
    void setupUi();
    void setInitialSelection(); 
    void setupHeader();
    void setupListWidget();
    void setupFooter();

    // Helper methods
    void initializeDefaults();
    void validateConfiguration();
    void animateSelection();
    void cleanupResources();

    // Error handling
    bool validateMenuOptions() const;
    void handleInvalidState();

private:
    // Core components
    SystemStateModel *m_stateModel;
    QStringList m_options;
    QListWidget *m_listWidget;

    // UI Components
    QVBoxLayout *m_mainLayout;
    QWidget *m_headerWidget;
    QWidget *m_footerWidget;
    QLabel *m_titleLabel;
    QLabel *m_descriptionLabel;
    QLabel *m_iconLabel;
    QLabel *m_navigationHintLabel;
    QFrame *m_separatorLine;

    // Configuration
    QString m_menuTitle;
    QString m_menuDescription;
    QPixmap m_menuIcon;
    bool m_navigationHintsEnabled;
    bool m_menuEnabled;

    // Auto-close functionality
    QTimer *m_autoCloseTimer;
    int m_autoCloseTimeout;

    // Constants
    static constexpr int DEFAULT_WIDTH = 300;
    static constexpr int DEFAULT_HEIGHT = 400;
    static constexpr int HEADER_HEIGHT = 60;
    static constexpr int FOOTER_HEIGHT = 60;
};

#endif // CUSTOMMENUDIALOG_H


