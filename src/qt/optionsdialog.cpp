#include "optionsdialog.h"
#include "optionsmodel.h"
#include "cryptobullionamountfield.h"
#include "monitoreddatamapper.h"
#include "guiutil.h"
#include "cryptobullionunits.h"
#include "qvaluecombobox.h"
#include "main.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QStackedWidget>

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QRegExpValidator>
#include <QDialogButtonBox>

extern bool fNoSpendZeroConfChange;
extern bool fNoSpendZeroConfChangeForced;
QCheckBox *use_change_address;
/* First page of options */
class MainOptionsPage : public QWidget
{
    Q_OBJECT
public:
    explicit MainOptionsPage(QWidget *parent=0);
    void setMapper(MonitoredDataMapper *mapper);
    QCheckBox *connect_socks4;
private:
    QCheckBox *cryptobullion_at_startup;
#ifndef Q_WS_MAC
    QCheckBox *minimize_to_tray;
#endif
    QCheckBox *map_port_upnp;
#ifndef Q_WS_MAC
    QCheckBox *minimize_on_close;
#endif    
    QCheckBox *no_spend_unconfirmed_change;
    QCheckBox *detach_database;
    QLineEdit *proxy_ip;
    QLineEdit *proxy_port;
    CryptobullionAmountField *fee_edit;

signals:

public slots:

};

class DisplayOptionsPage : public QWidget
{
    Q_OBJECT
public:
    explicit DisplayOptionsPage(QWidget *parent=0);
    void showRestartWarning_Lang();
    void setMapper(MonitoredDataMapper *mapper);
    bool fRestartWarningDisplayed_Lang;
    QValueComboBox *comboTranslationLang;
private:
    QValueComboBox *unit;
    QCheckBox *display_addresses;
    QCheckBox *coin_control_features;

signals:

public slots:

};

#include "optionsdialog.moc"

OptionsDialog::OptionsDialog(QWidget *parent):
    QDialog(parent), contents_widget(0), pages_widget(0),
    model(0), main_page(0), display_page(0),
    fRestartWarningDisplayed_Proxy(false),
    fRestartWarningDisplayed_Lang(false)
{
    contents_widget = new QListWidget();
    contents_widget->setMaximumWidth(128);

    pages_widget = new QStackedWidget();
    pages_widget->setMinimumWidth(300);

    QListWidgetItem *item_main = new QListWidgetItem(tr("Main"));
    contents_widget->addItem(item_main);
    main_page = new MainOptionsPage(this);
    pages_widget->addWidget(main_page);

    QListWidgetItem *item_display = new QListWidgetItem(tr("Display"));
    contents_widget->addItem(item_display);
    display_page = new DisplayOptionsPage(this);
    pages_widget->addWidget(display_page);

    contents_widget->setCurrentRow(0);

    QHBoxLayout *main_layout = new QHBoxLayout();
    main_layout->addWidget(contents_widget);
    main_layout->addWidget(pages_widget, 1);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addLayout(main_layout);

    QDialogButtonBox *buttonbox = new QDialogButtonBox();
    buttonbox->setStandardButtons(QDialogButtonBox::Apply|QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    apply_button = buttonbox->button(QDialogButtonBox::Apply);
    layout->addWidget(buttonbox);

    setLayout(layout);
    setWindowTitle(tr("Options"));

    /* Widget-to-option mapper */
    mapper = new MonitoredDataMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    mapper->setOrientation(Qt::Vertical);
    /* enable apply button when data modified */
    connect(mapper, SIGNAL(viewModified()), this, SLOT(enableApply()));
    /* disable apply button when new data loaded */
    connect(mapper, SIGNAL(currentIndexChanged(int)), this, SLOT(disableApply()));

    /* Event bindings */
    connect(contents_widget, SIGNAL(currentRowChanged(int)), this, SLOT(changePage(int)));
    connect(buttonbox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(buttonbox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(buttonbox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(applyClicked()));
}

void OptionsDialog::setModel(OptionsModel *model)
{
    this->model = model;

    mapper->setModel(model);
    main_page->setMapper(mapper);
    display_page->setMapper(mapper);

    mapper->toFirst();

    /* warn only when language selection changes by user action (placed here so init via mapper doesn't trigger this) */
    connect(display_page->comboTranslationLang, SIGNAL(valueChanged()), this, SLOT(showRestartWarning_Lang()));
    //connect(main_page->connect_socks4, SIGNAL(valueChanged()), this, SLOT(showRestartWarning_Proxy()));
    connect(main_page->connect_socks4, SIGNAL(toggled(bool)), this, SLOT(showRestartWarning_Proxy()));
}

void OptionsDialog::changePage(int index)
{
    pages_widget->setCurrentIndex(index);
}

void OptionsDialog::okClicked()
{
    mapper->submit();
    
    QSettings mySettings("CryptoBullion Foundation", "Vault");
    mySettings.beginGroup("UseChangeAddress");
    mySettings.setValue("UseChangeAddress", use_change_address->isChecked());
    mySettings.endGroup();

    fUseChangeAddress = use_change_address->isChecked();
    accept();
}

void OptionsDialog::cancelClicked()
{
    reject();
}

void OptionsDialog::showRestartWarning_Proxy()
{
    if(!fRestartWarningDisplayed_Proxy)
    {
        QMessageBox::warning(this, tr("Warning"), tr("This setting will take effect after restarting CBX Vault."), QMessageBox::Ok);
        fRestartWarningDisplayed_Proxy = true;
    }
}

void OptionsDialog::showRestartWarning_Lang()
{
    if(!fRestartWarningDisplayed_Lang)
    {
        QMessageBox::warning(this, tr("Warning"), tr("This setting will take effect after restarting CBX Vault."), QMessageBox::Ok);
        fRestartWarningDisplayed_Lang = true;
    }
}

void OptionsDialog::applyClicked()
{
    mapper->submit();
    apply_button->setEnabled(false);
}

void OptionsDialog::enableApply()
{
    apply_button->setEnabled(true);
}

void OptionsDialog::disableApply()
{
    apply_button->setEnabled(false);
}

MainOptionsPage::MainOptionsPage(QWidget *parent):
        QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout();

    cryptobullion_at_startup = new QCheckBox(tr("&Start CBX on window system startup"));
    cryptobullion_at_startup->setToolTip(tr("Automatically start CBX after the computer is turned on"));
    layout->addWidget(cryptobullion_at_startup);

#ifndef Q_WS_MAC
    minimize_to_tray = new QCheckBox(tr("&Minimize to the tray instead of the taskbar"));
    minimize_to_tray->setToolTip(tr("Show only a tray icon after minimizing the window"));
    layout->addWidget(minimize_to_tray);

    minimize_on_close = new QCheckBox(tr("M&inimize on close"));
    minimize_on_close->setToolTip(tr("Minimize instead of exit the application when the window is closed. When this option is enabled, the application will be closed only after selecting Quit in the menu."));
    layout->addWidget(minimize_on_close);
#endif

    map_port_upnp = new QCheckBox(tr("Map port using &UPnP"));
    map_port_upnp->setToolTip(tr("Automatically open the CBX client port on the router. This only works when your router supports UPnP and it is enabled."));
    layout->addWidget(map_port_upnp);

    connect_socks4 = new QCheckBox(tr("&Connect through SOCKS4 proxy:"));
    connect_socks4->setToolTip(tr("Connect to the CBX network through a SOCKS4 proxy (e.g. when connecting through Tor)"));
    layout->addWidget(connect_socks4);

    QHBoxLayout *proxy_hbox = new QHBoxLayout();
    proxy_hbox->addSpacing(18);
    QLabel *proxy_ip_label = new QLabel(tr("Proxy &IP: "));
    proxy_hbox->addWidget(proxy_ip_label);
    proxy_ip = new QLineEdit();
    proxy_ip->setMaximumWidth(140);
    proxy_ip->setEnabled(false);
    proxy_ip->setValidator(new QRegExpValidator(QRegExp("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}"), this));
    proxy_ip->setToolTip(tr("IP address of the proxy (e.g. 127.0.0.1)"));
    proxy_ip_label->setBuddy(proxy_ip);
    proxy_hbox->addWidget(proxy_ip);
    QLabel *proxy_port_label = new QLabel(tr("&Port: "));
    proxy_hbox->addWidget(proxy_port_label);
    proxy_port = new QLineEdit();
    proxy_port->setMaximumWidth(55);
    proxy_port->setValidator(new QIntValidator(0, 65535, this));
    proxy_port->setEnabled(false);
    proxy_port->setToolTip(tr("Port of the proxy (e.g. 1234)"));
    proxy_port_label->setBuddy(proxy_port);
    proxy_hbox->addWidget(proxy_port);
    proxy_hbox->addStretch(1);
    layout->addLayout(proxy_hbox);

    no_spend_unconfirmed_change = new QCheckBox(tr("&Disable spending unconfirmed change:"));
    no_spend_unconfirmed_change->setToolTip(tr("If you disable the spending of unconfirmed change, the change from a transaction cannot be used until that transaction has at least one confirmation. This also affects how your balance is computed."));
    layout->addWidget(no_spend_unconfirmed_change);

    use_change_address = new QCheckBox(tr("&Use change address (not recommended)"));
    use_change_address->setChecked(fUseChangeAddress);
    layout->addWidget(use_change_address);

    QLabel *fee_help = new QLabel(tr("Mandatory network transaction fee per kB transferred. Most transactions are 1 kB and incur a 0.001 CBX fee. Note: transfer size may increase depending on the number of input transactions required to be added together to fund the payment."));
    fee_help->setWordWrap(true);
    layout->addWidget(fee_help);

    QHBoxLayout *fee_hbox = new QHBoxLayout();
    fee_hbox->addSpacing(18);
    QLabel *fee_label = new QLabel(tr("Additional network &fee:"));
    fee_hbox->addWidget(fee_label);
    fee_edit = new CryptobullionAmountField();
    fee_edit->setDisabled(true);

    fee_label->setBuddy(fee_edit);
    fee_hbox->addWidget(fee_edit);
    fee_hbox->addStretch(1);

    layout->addLayout(fee_hbox);

    detach_database = new QCheckBox(tr("Detach databases at shutdown"));
    detach_database->setToolTip(tr("Detach block and address databases at shutdown. This means they can be moved to another data directory, but it slows down shutdown. The wallet is always detached."));
    layout->addWidget(detach_database);

    layout->addStretch(1); // Extra space at bottom

    setLayout(layout);

    connect(connect_socks4, SIGNAL(toggled(bool)), proxy_ip, SLOT(setEnabled(bool)));
    connect(connect_socks4, SIGNAL(toggled(bool)), proxy_port, SLOT(setEnabled(bool)));    

#ifndef USE_UPNP
    map_port_upnp->setEnabled(true);
#endif

    if (fNoSpendZeroConfChangeForced)
    {
        no_spend_unconfirmed_change->setChecked(true);
        no_spend_unconfirmed_change->setDisabled(true);
    }
}

void MainOptionsPage::setMapper(MonitoredDataMapper *mapper)
{
    // Map model to widgets
    mapper->addMapping(cryptobullion_at_startup, OptionsModel::StartAtStartup);
#ifndef Q_WS_MAC
    mapper->addMapping(minimize_to_tray, OptionsModel::MinimizeToTray);
#endif
    mapper->addMapping(map_port_upnp, OptionsModel::MapPortUPnP);
    // do not map setting for no_spend_unconfirmed_change if that's been forced by commandline arg
    if (!fNoSpendZeroConfChangeForced)
        mapper->addMapping(no_spend_unconfirmed_change, OptionsModel::NoSpendUnconfirmedChange);
#ifndef Q_WS_MAC
    mapper->addMapping(minimize_on_close, OptionsModel::MinimizeOnClose);
#endif
    mapper->addMapping(connect_socks4, OptionsModel::ProxyUse);
    mapper->addMapping(proxy_ip, OptionsModel::ProxyIP);
    mapper->addMapping(proxy_port, OptionsModel::ProxyPort);
    mapper->addMapping(fee_edit, OptionsModel::Fee);
    mapper->addMapping(detach_database, OptionsModel::DetachDatabases);

    // warning handler for proxy setting change
    connect(connect_socks4, SIGNAL(clicked(bool)), this, SLOT(showRestartWarning_Proxy()));
}

DisplayOptionsPage::DisplayOptionsPage(QWidget *parent):
        QWidget(parent),
        fRestartWarningDisplayed_Lang(false)
{
    QVBoxLayout *layout = new QVBoxLayout();

    QHBoxLayout *unit_hbox = new QHBoxLayout();
    unit_hbox->addSpacing(18);
    QLabel *unit_label = new QLabel(tr("&Unit to show amounts in: "));
    unit_hbox->addWidget(unit_label);
    unit = new QValueComboBox(this);
    unit->setModel(new CryptobullionUnits(this));
    unit->setToolTip(tr("Choose the default subdivision unit to show in the interface, and when sending coins"));

    unit_label->setBuddy(unit);
    unit_hbox->addWidget(unit);

    layout->addLayout(unit_hbox);

    // translation
    QHBoxLayout *trans_hbox = new QHBoxLayout();
    trans_hbox->addSpacing(18);

    QLabel *trans_label = new QLabel(tr("&Interface Language: "));
    trans_hbox->addWidget(trans_label);
    comboTranslationLang = new QValueComboBox(this);
    /* warn only when language selection changes by user action (placed here so init via mapper doesn't trigger this) */
    connect(comboTranslationLang, SIGNAL(valueChanged()), this, SLOT(showRestartWarning_Lang()));
    //comboTranslationLang->setModel(new CryptobullionUnits(this));

    QDir translations(":translations");
    comboTranslationLang->addItem(QString("(") + tr("default") + QString(")"), QVariant(""));

    foreach(const QString &langStr, translations.entryList())
    {
        QLocale locale(langStr);

        // check if the locale name consists of 2 parts (language_country)
        if(langStr.contains("_"))
        {
#if QT_VERSION >= 0x040800
            // display language strings as "native language - native country (locale name)", e.g. "Deutsch - Deutschland (de)"
            comboTranslationLang->addItem(locale.nativeLanguageName() + QString(" - ") + locale.nativeCountryName() + QString(" (") + langStr + QString(")"), QVariant(langStr));
#else
            // display language strings as "language - country (locale name)", e.g. "German - Germany (de)"
            comboTranslationLang->addItem(QLocale::languageToString(locale.language()) + QString(" - ") + QLocale::countryToString(locale.country()) + QString(" (") + langStr + QString(")"), QVariant(langStr));
#endif
        }
        else
        {
#if QT_VERSION >= 0x040800
            // display language strings as "native language (locale name)", e.g. "Deutsch (de)"
            comboTranslationLang->addItem(locale.nativeLanguageName() + QString(" (") + langStr + QString(")"), QVariant(langStr));
#else
            // display language strings as "language (locale name)", e.g. "German (de)"
            comboTranslationLang->addItem(QLocale::languageToString(locale.language()) + QString(" (") + langStr + QString(")"), QVariant(langStr));
#endif
        }
    }

    comboTranslationLang->setToolTip(tr("Choose the interface language."));

    trans_label->setBuddy(comboTranslationLang);
    trans_hbox->addWidget(comboTranslationLang);

    layout->addLayout(trans_hbox);

    display_addresses = new QCheckBox(tr("&Display addresses in transaction list"), this);
    display_addresses->setToolTip(tr("Whether to show CBX addresses in the transaction list"));
    layout->addWidget(display_addresses);

    coin_control_features = new QCheckBox(tr("Display bullion control features (experts only!)"), this);
    coin_control_features->setToolTip(tr("Whether to show bullion control features or not"));
    layout->addWidget(coin_control_features);

    layout->addStretch();

    setLayout(layout);
}

void DisplayOptionsPage::showRestartWarning_Lang()
{
    if(!fRestartWarningDisplayed_Lang)
    {
        QMessageBox::warning(this, tr("Warning"), tr("This setting will take effect after restarting CBX Vault."), QMessageBox::Ok);
        fRestartWarningDisplayed_Lang = true;
    }
}

void DisplayOptionsPage::setMapper(MonitoredDataMapper *mapper)
{
    mapper->addMapping(unit, OptionsModel::DisplayUnit);
    mapper->addMapping(display_addresses, OptionsModel::DisplayAddresses);
    mapper->addMapping(coin_control_features, OptionsModel::CoinControlFeatures);
    mapper->addMapping(comboTranslationLang, OptionsModel::Language);
}
