#include <QMenu>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickView>
#include <QSystemTrayIcon>
#include <QtDebug>

#include "../components/contacts/ContactsListProxyModel.hpp"
#include "../components/linphone/LinphoneCore.hpp"
#include "../components/notification/Notification.hpp"
#include "../components/settings/AccountSettingsModel.hpp"
#include "../components/timeline/TimelineModel.hpp"

#include "App.hpp"

#define LANGUAGES_PATH ":/languages/"
#define WINDOW_ICON_PATH ":/assets/images/linphone.png"

// ===================================================================

App::App (int &argc, char **argv) : QApplication(argc, argv) {
  // Try to use default locale. Otherwise use english.
  if (m_translator.load(QString(LANGUAGES_PATH) + QLocale::system().name()) ||
      m_translator.load(LANGUAGES_PATH "en")) {
    installTranslator(&m_translator);
  } else {
    qWarning("No translation found.");
  }

  setWindowIcon(QIcon(WINDOW_ICON_PATH));

  // Provide `+custom` folders for custom components.
  m_file_selector = new QQmlFileSelector(&m_engine);
  m_file_selector->setExtraSelectors(QStringList("custom"));

  // Set modules paths.
  m_engine.addImportPath(":/ui/modules");
  m_engine.addImportPath(":/ui/scripts");
  m_engine.addImportPath(":/ui/views");

  // Register types and load context properties.
  registerTypes();
  addContextProperties();

  // Load main view.
  m_engine.load(QUrl("qrc:/ui/views/App/MainWindow/MainWindow.qml"));
  if (m_engine.rootObjects().isEmpty())
    qFatal("Unable to open main window.");

  // Enable TrayIconSystem.
  if (!QSystemTrayIcon::isSystemTrayAvailable())
    qWarning("System tray not found on this system.");
  else
    setTrayIcon();
}

// -------------------------------------------------------------------

void App::registerTypes () {
  qmlRegisterUncreatableType<Presence>(
    "Linphone", 1, 0, "Presence", "Presence is uncreatable"
  );

  ContactsListProxyModel::initContactsListModel(new ContactsListModel());
  qmlRegisterType<ContactsListProxyModel>("Linphone", 1, 0, "ContactsListProxyModel");

  // Expose the static functions of ContactsListModel.
  qmlRegisterSingletonType<ContactsListModel>(
    "Linphone", 1, 0, "ContactsListModel",
    [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject *{
      Q_UNUSED(engine);
      Q_UNUSED(scriptEngine);

      return ContactsListProxyModel::getContactsListModel();
    }
  );
}

void App::addContextProperties () {
  QQmlContext *context = m_engine.rootContext();
  QQmlComponent component(&m_engine, QUrl("qrc:/ui/views/App/Calls/Calls.qml"));

  // Windows.
  if (component.isError()) {
    qWarning() << component.errors();
  } else {
    // context->setContextProperty("CallsWindow", component.create());
  }

  // Models.
  context->setContextProperty("AccountSettingsModel", new AccountSettingsModel());
  context->setContextProperty("TimelineModel", new TimelineModel());

  // Other.
  context->setContextProperty("LinphoneCore", LinphoneCore::getInstance());
  context->setContextProperty("Notification", new Notification());
}

void App::setTrayIcon () {
  QQuickWindow *root = qobject_cast<QQuickWindow *>(m_engine.rootObjects().at(0));
  QMenu *menu = new QMenu();
  QSystemTrayIcon *tray_icon = new QSystemTrayIcon(root);

  // trayIcon: Right click actions.
  QAction *quit_action = new QAction("Quit", root);
  root->connect(quit_action, &QAction::triggered, qApp, &QCoreApplication::quit);

  QAction *restore_action = new QAction("Restore", root);
  root->connect(restore_action, &QAction::triggered, root, &QQuickWindow::showNormal);

  // trayIcon: Left click actions.
  root->connect(tray_icon, &QSystemTrayIcon::activated, [root](QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
      if (root->visibility() == QWindow::Hidden)
        root->showNormal();
      else
        root->hide();
    }
  });

  // Build trayIcon menu.
  menu->addAction(restore_action);
  menu->addSeparator();
  menu->addAction(quit_action);

  tray_icon->setContextMenu(menu);
  tray_icon->setIcon(QIcon(WINDOW_ICON_PATH));
  tray_icon->setToolTip("Linphone");
  tray_icon->show();
}
