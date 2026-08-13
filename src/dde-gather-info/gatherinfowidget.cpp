#include "gatherinfowidget.h"

#include <QPainter>
#include <QFile>
#include <QSsl>
#include <QJsonDocument>
#include <QJsonObject>
#include <DConfig>
#include <QPainterPath>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusInterface>
#include <DGuiApplicationHelper>
#include <QWebEngineProfile>
#include <QWebChannel>
#include <QWebEngineSettings>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QSslCertificate>
#include <QSslConfiguration>

JsBridge::JsBridge(QObject *parent)
    : QObject(parent)
{

}

JsBridge::~JsBridge()
{

}

void JsBridge::UpdateDeviceCancel()
{
    qInfo() << "Received UpdateDeviceCancel from JavaScript";
    Q_EMIT sigUpdateDeviceCancel();
}

void JsBridge::UpdateDeviceSuccess()
{
    qInfo() << "Received UpdateDeviceSuccess from JavaScript";
    Q_EMIT sigUpdateDeviceSuccess();
}

void JsBridge::WidgetCanShow()
{
    qInfo() << "Received WidgetCanShow from JavaScript";
    Q_EMIT sigWidgetCanShow();
}

CustomWebEnginePage::CustomWebEnginePage(QObject *parent)
    : QWebEnginePage(parent)
{
    ignoreCertificateErrors();
    loadCertificate("/usr/local/share/ca-certificates/uniontech-iup-root-ca.crt");
}

void CustomWebEnginePage::ignoreCertificateErrors()
{
    connect(this, &QWebEnginePage::certificateError, this, [](const QWebEngineCertificateError &) {
        // ignore all certificate errors
    });
}

CustomWebEnginePage::~CustomWebEnginePage()
{

}

void CustomWebEnginePage::loadCertificate(const QString &filepath)
{
    QFile certFile(filepath);
    if (!certFile.open(QIODevice::ReadOnly)) {
        qWarning() << "cannot open crt file" << filepath;
        return;
    }

    QSslCertificate certificate(&certFile, QSsl::EncodingFormat::Pem);
    certFile.close();

    QList<QSslCertificate> certs = QSslConfiguration::defaultConfiguration().caCertificates();
    certs.append(certificate);
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setCaCertificates(certs);
    QSslConfiguration::setDefaultConfiguration(sslConfig);
}

ContentWidget::ContentWidget(QWidget *parent)
    :QWidget(parent)
{
    setFixedSize(520, 700);
    QPalette contentPalette = palette();
    contentPalette.setColor(QPalette::Window, Qt::white);
    setPalette(contentPalette);
}

ContentWidget::~ContentWidget()
{

}

void ContentWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    painter.setBrush(this->palette().window());
    painter.drawRect(this->rect());
}

GatherInfoWidget::GatherInfoWidget(QWidget* parent)
    : QFrame(parent)
{
    setWindowFlags(Qt::Dialog);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);

    m_url = QUrl(QString("%1/update-device-info?%2").arg(getDomainName()).arg(makeUrlVal()));
    qInfo() << "load web page " << m_url;
    m_webPage = new CustomWebEnginePage(this);
    m_webView = new QWebEngineView(this);
    m_webView->setPage(m_webPage);
    m_webPage->settings()->setAttribute(QWebEngineSettings::ShowScrollBars, false);
    m_webView->setFixedSize(500, 646);
    m_webView->load(m_url);

    m_jsBridget = new JsBridge(this);
    QWebChannel *channel = new QWebChannel(this);
    channel->registerObject("client", m_jsBridget);
    m_webView->page()->setWebChannel(channel);

    QFile scriptFile(":/qtwebchannel/qwebchannel.js");
    scriptFile.open(QIODevice::ReadOnly);
    QString apiScript = QString::fromLatin1(scriptFile.readAll());

    QWebEngineScript script;
    script.setSourceCode(apiScript);
    script.setName("qwebchannel.js");
    script.setWorldId(QWebEngineScript::MainWorld);
    script.setInjectionPoint(QWebEngineScript::DocumentReady);
    script.setRunsOnSubFrames(false);
    m_webPage->profile()->scripts()->insert(script);

    connect(m_jsBridget, &JsBridge::sigUpdateDeviceCancel, this, [&]() {
        QCoreApplication::quit();
    });

    connect(m_jsBridget, &JsBridge::sigUpdateDeviceSuccess, this, [&]() {
        QCoreApplication::quit();
    });

    connect(m_jsBridget, &JsBridge::sigWidgetCanShow, this, [this]() {
        m_canShow = true;
        Q_EMIT sigWidgetCanShow();
    });

    QObject::connect(&m_timer, &QTimer::timeout, [this]() {
        if (m_canShow) return;
        qInfo() << "load web page out of time";
        QCoreApplication::quit();
    });
    m_timer.start(300000);

    m_closeBtn = new QPushButton(this);
    QIcon icon(":/icons/window-close.svg");
    m_closeBtn->setIcon(icon);
    m_closeBtn->setFixedSize(16, 16);
    m_closeBtn->setIconSize(QSize(16, 16));

    connect(m_closeBtn, &QPushButton::clicked, this, [&]() {
        QCoreApplication::quit();
    });

    m_contentWidget = new ContentWidget(this);
    QHBoxLayout *closeBtnLayout = new QHBoxLayout();
    closeBtnLayout->addStretch();
    closeBtnLayout->addWidget(m_closeBtn, Qt::AlignRight);
    QHBoxLayout *webViewLayout = new QHBoxLayout();
    webViewLayout->addWidget(m_webView, Qt::AlignCenter);
    QVBoxLayout *contentLayout = new QVBoxLayout();
    contentLayout->addLayout(closeBtnLayout);
    contentLayout->addLayout(webViewLayout);
    m_contentWidget->setLayout(contentLayout);

    QHBoxLayout *frameHLayout = new QHBoxLayout();
    frameHLayout->addWidget(m_contentWidget, Qt::AlignCenter);
    QVBoxLayout *frameVLayout = new QVBoxLayout();
    frameVLayout->addLayout(frameHLayout);
    setLayout(frameVLayout);
}

GatherInfoWidget::~GatherInfoWidget()
{
}

QString GatherInfoWidget::getHostName()
{
    QDBusInterface hostname_ifc_(
                "org.freedesktop.hostname1",
                "/org/freedesktop/hostname1",
                "org.freedesktop.hostname1",
                QDBusConnection::systemBus()
                );
    qDebug() << "connect" << "org.freedesktop.hostname1" << hostname_ifc_.isValid();
    if (!hostname_ifc_.isValid())
        return QString("");

    return QString::fromUtf8(QUrl::toPercentEncoding(hostname_ifc_.property("Hostname").toString()));
}

QString GatherInfoWidget::getDomainName()
{
    QFile file("/var/lib/lastore-private/config_override.json");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray fileContent = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(fileContent);
        if (!doc.isEmpty()) {
            QJsonObject jsonObj = doc.object();
            QString domainUrl = jsonObj["PlatformUrl"].toString();
            if (domainUrl.contains("/iup-api")) {
                domainUrl.remove("/iup-api");
            }
            if (!domainUrl.startsWith("https://", Qt::CaseInsensitive)) {
                qWarning() << "PlatformUrl must use HTTPS, got:" << domainUrl;
                return "";
            }
            return domainUrl;
        }
    }

    qWarning() << "Failed to read config_override.json, fallback to dconfig";
    Dtk::Core::DConfig *dconfig = Dtk::Core::DConfig::create("org.deepin.dde.lastore", "org.deepin.dde.lastore", "", this);
    if (dconfig && dconfig->isValid()) {
        QString platformUrl = dconfig->value("platform-url").toString();
        if (!platformUrl.isEmpty()) {
            if (!platformUrl.startsWith("https://", Qt::CaseInsensitive)) {
                qWarning() << "platform-url from dconfig must use HTTPS, got:" << platformUrl;
                return "";
            }
            if (platformUrl.contains("/iup-api")) {
                platformUrl.remove("/iup-api");
            }
            return platformUrl;
        }
    }
    qWarning() << "Failed to get domain name from dconfig";
    return "";
}

QString GatherInfoWidget::getStandardFont()
{
    QDBusInterface appearance_ifc_(
                "com.deepin.daemon.Appearance",
                "/com/deepin/daemon/Appearance",
                "com.deepin.daemon.Appearance",
                QDBusConnection::sessionBus()
                );
    qDebug() << "connect" << "com.deepin.daemon.Appearance" << appearance_ifc_.isValid();
    if (!appearance_ifc_.isValid())
        return QString("");

    return QString::fromUtf8(QUrl::toPercentEncoding(appearance_ifc_.property("StandardFont").toString()));
}

QString GatherInfoWidget::get99Token()
{
    QString filePath = "/etc/apt/apt.conf.d/99lastore-token.conf";

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "cannot open file:" << file.errorString();
        return "";
    }

    QTextStream in(&file);
    QString fileContent = in.readAll();

    int startIndex = fileContent.indexOf('"') + 1;
    int endIndex = fileContent.lastIndexOf('"');
    QString token = fileContent.mid(startIndex, endIndex - startIndex);

    return QString::fromUtf8(QUrl::toPercentEncoding(token));
}

QString GatherInfoWidget::getTheme()
{
    auto themeType = Dtk::Gui::DGuiApplicationHelper::instance()->themeType();

    switch (themeType) {
    case Dtk::Gui::DGuiApplicationHelper::DarkType:
        return "dark";
    case Dtk::Gui::DGuiApplicationHelper::LightType:
    case Dtk::Gui::DGuiApplicationHelper::UnknownType:
    default:
        return "light";
    }
}

QString GatherInfoWidget::makeUrlVal()
{
    QString urlVal;
    urlVal += "token=";
    urlVal += get99Token();
    urlVal += "&theme=";
    urlVal += getTheme();
    urlVal += "&font=";
    urlVal += getStandardFont();
    urlVal += "&hostname=";
    urlVal += getHostName();
    QDateTime currentDateTime = QDateTime::currentDateTime();
    qint64 timestamp = currentDateTime.toSecsSinceEpoch();
    urlVal += "&timestamp=";
    urlVal += QString::number(timestamp);

    return urlVal;
}
