#include "gatherinfowidget.h"
#include "fullscreen.h"
#include "fullscreenmanager.h"
#include "fullscreenbackground.h"

#include <QApplication>
#include <QSurfaceFormat>
#include <DApplication>
#include <DLog>

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[])
{
    QStringList chromiumFlags;
#ifdef __sw_64__
    chromiumFlags << "--no-sandbox";
#endif

    if (qgetenv("XDG_SESSION_TYPE").contains("wayland")) {
        QSurfaceFormat format;
        format.setRenderableType(QSurfaceFormat::OpenGLES);
        QSurfaceFormat::setDefaultFormat(format);
        qputenv("QT_WAYLAND_SHELL_INTEGRATION", "kwayland-shell");
        // DTK6 removed loadDXcbPlugin
    }

    //Disable function: Qt::AA_ForceRasterWidgets, solve the display problem of domestic platform (loongson mips)
    chromiumFlags << "--disable-gpu" << "--disable-web-security" << "--single-process" << "--ignore-certificate-errors";
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", chromiumFlags.join(' ').toUtf8());
    qputenv("DXCB_FAKE_PLATFORM_NAME_XCB", "true");

    // 系统使用自动代理时，程序会闪退，不知道原因暂时规避
    const QString auto_proxy = qgetenv("auto_proxy");
    if (!auto_proxy.isEmpty()) {
        qputenv("auto_proxy", "");
    }

    //龙芯机器配置,使得DApplication能正确加载QTWEBENGINE
    qputenv("DTK_FORCE_RASTER_WIDGETS", "FALSE");

    qputenv("_d_disableDBusFileDialog", "true");
    setenv("PULSE_PROP_media.role", "video", 1);

    QString glstring = QString::fromLocal8Bit(qgetenv("DEBUG_OPENGL"));
    if(!glstring.isEmpty())
    {
        QSurfaceFormat format;
        int gltype = glstring.toInt();
        qDebug() << "set gltype:" << gltype;
        if(gltype == 1)
        {
            format.setRenderableType(QSurfaceFormat::DefaultRenderableType);
        }
        else if (gltype == 2)
        {
            format.setRenderableType(QSurfaceFormat::OpenGL);
        }
        else if (gltype == 3)
        {
            format.setRenderableType(QSurfaceFormat::OpenGLES);
        }

        qDebug() << "set surface format:" << format.renderableType();
        QSurfaceFormat::setDefaultFormat(format);
    }

    Dtk::Widget::DApplication app(argc, argv);
    GatherInfoWidget gatherInfoWidget;

    DLogManager::setLogFormat("%{time}{yyyy-MM-dd, HH:mm:ss.zzz} [%{type:-7}] [ %{function:-35} %{line}] %{message}\n");
    DLogManager::registerConsoleAppender();

    auto createFrame = [&gatherInfoWidget](QPointer<QScreen> screen) -> QWidget * {
        // 所有的界面共用一块内容
        FullScreenBackground *bg = new FullScreenBackground();
        FullScreenBackground::setContent(&gatherInfoWidget);

        bg->setScreen(screen);
        return bg;
    };

   FullScreenManager fullScreenWindow(createFrame);

    QObject::connect(&gatherInfoWidget, &GatherInfoWidget::sigWidgetCanShow, &fullScreenWindow, &FullScreenManager::screenCountChanged);

    return app.exec();
}
