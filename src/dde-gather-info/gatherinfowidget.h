#ifndef GATHERINFOWIDGET_H
#define GATHERINFOWIDGET_H

#include <QMainWindow>

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineCertificateError>
#include <QTimer>

class JsBridge : public QObject
{
    Q_OBJECT
public:
    explicit JsBridge(QObject *parent = nullptr);
    ~JsBridge();

Q_SIGNALS:
    void sigUpdateDeviceCancel();
    void sigUpdateDeviceSuccess();
    void sigWidgetCanShow();

public Q_SLOTS:
    void UpdateDeviceCancel();
    void UpdateDeviceSuccess();
    void WidgetCanShow();
};

class CustomWebEnginePage : public QWebEnginePage {
    Q_OBJECT

public:
    explicit CustomWebEnginePage(QObject *parent = nullptr);
    ~CustomWebEnginePage();

    void ignoreCertificateErrors();

protected:
    virtual void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString& message, int lineNumber, const QString& sourceID) override {
        qWarning() << "Javascript Message:" << message
                   << "line number:" << QString::number(lineNumber)
                   << "sourceID" << sourceID;
    }

private:
    void loadCertificate(const QString &filepath);
};

class ContentWidget : public QWidget
{
    Q_OBJECT

public:
    ContentWidget(QWidget *parent);
    ~ContentWidget();

protected:
    void paintEvent(QPaintEvent *event) override;
};

class GatherInfoWidget : public QFrame
{
    Q_OBJECT

public:
    GatherInfoWidget(QWidget *parent = nullptr);
    ~GatherInfoWidget();

Q_SIGNALS:
    void sigWidgetCanShow();

private:
    QString getHostName();
    QString getDomainName();
    QString getStandardFont();
    QString get99Token();
    QString getTheme();
    QString makeUrlVal();

private:
    QWebEngineView* m_webView = nullptr;
    CustomWebEnginePage* m_webPage = nullptr;
    QUrl m_url;
    JsBridge* m_jsBridget;
    QPushButton* m_closeBtn = nullptr;
    ContentWidget* m_contentWidget = nullptr;
    QTimer m_timer;
    bool m_canShow = false;
};

#endif // GATHERINFOWIDGET_H
