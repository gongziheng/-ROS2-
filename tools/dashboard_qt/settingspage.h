#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QUrl>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);
    ~SettingsPage() override = default;

    void setEndpoints(const QUrl &apiBaseUrl, const QUrl &wsUrl);

signals:
    void endpointsApplied(const QString &apiBaseUrl, const QString &wsUrl);

private slots:
    void onApplyClicked();
    void onRestoreDefaultsClicked();

private:
    QString normalizedApiBaseUrl(const QString &text) const;
    QString normalizedWsUrl(const QString &text) const;

private:
    QLineEdit *m_apiBaseEdit = nullptr;
    QLineEdit *m_wsEdit = nullptr;
    QLabel *m_hintLabel = nullptr;
    QPushButton *m_applyButton = nullptr;
    QPushButton *m_restoreButton = nullptr;
};

#endif // SETTINGSPAGE_H