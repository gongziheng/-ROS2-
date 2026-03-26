#include "settingspage.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 24, 24, 24);
    rootLayout->setSpacing(16);

    auto *titleLabel = new QLabel(tr("连接设置"), this);
    titleLabel->setStyleSheet("font-size: 22px; font-weight: 700;");

    auto *descLabel = new QLabel(
        tr("第二版只保留最小可用设置：平台 API 地址和 WebSocket 地址。"
           "API Base URL 会自动拼接成 /api/status、/api/tasks、/api/tasks/recent、/api/alerts/recent。"),
        this);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: #5f6b7a;");

    auto *groupBox = new QGroupBox(tr("平台地址"), this);
    auto *formLayout = new QFormLayout(groupBox);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    formLayout->setHorizontalSpacing(16);
    formLayout->setVerticalSpacing(14);

    m_apiBaseEdit = new QLineEdit(groupBox);
    m_apiBaseEdit->setPlaceholderText("http://127.0.0.1:8000");

    m_wsEdit = new QLineEdit(groupBox);
    m_wsEdit->setPlaceholderText("ws://127.0.0.1:8000/ws/status");

    formLayout->addRow(tr("API Base URL"), m_apiBaseEdit);
    formLayout->addRow(tr("WebSocket URL"), m_wsEdit);

    m_hintLabel = new QLabel(
        tr("示例：API Base URL = http://127.0.0.1:8000，"
           "WebSocket URL = ws://127.0.0.1:8000/ws/status"),
        groupBox);
    m_hintLabel->setWordWrap(true);
    m_hintLabel->setStyleSheet("color: #7a8696; font-size: 12px;");
    formLayout->addRow(QString(), m_hintLabel);

    m_statusLabel = new QLabel(groupBox);
    m_statusLabel->setWordWrap(true);
    formLayout->addRow(tr("状态"), m_statusLabel);

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_restoreButton = new QPushButton(tr("恢复默认"), this);
    m_applyButton = new QPushButton(tr("应用"), this);

    m_restoreButton->setMinimumWidth(120);
    m_applyButton->setMinimumWidth(120);

    buttonLayout->addWidget(m_restoreButton);
    buttonLayout->addWidget(m_applyButton);

    rootLayout->addWidget(titleLabel);
    rootLayout->addWidget(descLabel);
    rootLayout->addWidget(groupBox);
    rootLayout->addLayout(buttonLayout);
    rootLayout->addStretch();

    connect(m_applyButton, &QPushButton::clicked,
            this, &SettingsPage::onApplyClicked);
    connect(m_restoreButton, &QPushButton::clicked,
            this, &SettingsPage::onRestoreDefaultsClicked);

    setEndpoints(QUrl("http://127.0.0.1:8000"),
                 QUrl("ws://127.0.0.1:8000/ws/status"));
    setStatusMessage(tr("当前为默认连接配置。"), true);
}

void SettingsPage::setEndpoints(const QUrl &apiBaseUrl, const QUrl &wsUrl)
{
    m_apiBaseEdit->setText(apiBaseUrl.toString());
    m_wsEdit->setText(wsUrl.toString());
}

void SettingsPage::setStatusMessage(const QString &text, bool ok)
{
    if (!m_statusLabel) {
        return;
    }

    m_statusLabel->setText(text);
    if (ok) {
        m_statusLabel->setStyleSheet("color: #2e7d32; font-size: 12px;");
    } else {
        m_statusLabel->setStyleSheet("color: #c62828; font-size: 12px;");
    }
}

QString SettingsPage::normalizedApiBaseUrl(const QString &text) const
{
    QString value = text.trimmed();
    while (value.endsWith('/')) {
        value.chop(1);
    }
    return value;
}

QString SettingsPage::normalizedWsUrl(const QString &text) const
{
    QString value = text.trimmed();
    while (value.endsWith('/')) {
        value.chop(1);
    }

    if (value.startsWith("http://")) {
        value.replace(0, 7, "ws://");
    } else if (value.startsWith("https://")) {
        value.replace(0, 8, "wss://");
    }
    return value;
}

void SettingsPage::onApplyClicked()
{
    const QString apiBaseUrl = normalizedApiBaseUrl(m_apiBaseEdit->text());
    const QString wsUrl = normalizedWsUrl(m_wsEdit->text());

    if (apiBaseUrl.isEmpty() || wsUrl.isEmpty()) {
        setStatusMessage(tr("请输入 API Base URL 和 WebSocket URL。"), false);
        return;
    }

    setStatusMessage(tr("已提交连接配置，等待主窗口应用。"), true);
    emit endpointsApplied(apiBaseUrl, wsUrl);
}

void SettingsPage::onRestoreDefaultsClicked()
{
    setEndpoints(QUrl("http://127.0.0.1:8000"),
                 QUrl("ws://127.0.0.1:8000/ws/status"));
    setStatusMessage(tr("已恢复默认地址，点击“应用”后生效。"), true);
}