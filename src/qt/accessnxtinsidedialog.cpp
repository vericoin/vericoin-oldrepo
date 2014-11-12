#include "accessnxtinsidedialog.h"
#include "ui_accessnxtinsidedialog.h"

#include "addressbookpage.h"
#include "base58.h"
#include "guiutil.h"
#include "init.h"
#include "main.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "wallet.h"
#include "util.h"

#include <string>
#include <vector>

#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QString>
#include <QUrlQuery>

AccessNxtInsideDialog::AccessNxtInsideDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AccessNxtInsideDialog),
    model(0)
{
    ui->setupUi(this);

#if (QT_VERSION >= 0x040700)
    /* Do not move this to the XML file, Qt before 4.7 will choke on it */
    ui->addressIn_AN->setPlaceholderText(tr("Enter a VeriCoin address (e.g. VTHZfUg11wEJmSgBLUcmCKGYekuqFcGHQq)"));
#endif

    GUIUtil::setupAddressWidget(ui->addressIn_AN, this);

    ui->addressIn_AN->installEventFilter(this);
	ui->addressIn_AN->setFocus();
}

AccessNxtInsideDialog::~AccessNxtInsideDialog()
{
    delete ui;
}

void AccessNxtInsideDialog::setModel(WalletModel *model)
{
    this->model = model;
}

void AccessNxtInsideDialog::setAddress_AN(QString address)
{
    ui->addressIn_AN->setText(address);
	ui->statusLabel_AN->clear();
	ui->accessNxtInsideButton_AN->setFocus();
}

void AccessNxtInsideDialog::showTab_AN(bool fShow)
{
    ui->tabWidget->setCurrentIndex(0);
	ui->statusLabel_AN->clear();

    if (fShow)
        this->show();
}

void AccessNxtInsideDialog::on_addressBookButton_AN_clicked()
{
    if (model && model->getAddressTableModel())
    {
        AddressBookPage dlg(AddressBookPage::ForSending, AddressBookPage::ReceivingTab, this);
        dlg.setModel(model->getAddressTableModel());
        if (dlg.exec())
        {
            setAddress_AN(dlg.getReturnValue());
        }
    }
}

void AccessNxtInsideDialog::on_pasteButton_AN_clicked()
{
    setAddress_AN(QApplication::clipboard()->text());
}

void AccessNxtInsideDialog::on_accessNxtInsideButton_AN_clicked()
{
    /* Clear old signature to ensure users don't get confused on error with an old signature displayed */
	ui->statusLabel_AN->clear();

    CBitcoinAddress addr(ui->addressIn_AN->text().toStdString());
    if (!addr.IsValid())
    {
        ui->addressIn_AN->setValid(false);
        ui->statusLabel_AN->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_AN->setText(tr("The entered address is invalid.") + QString(" ") + tr("Please check the address and try again."));
        return;
    }
    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
    {
        ui->addressIn_AN->setValid(false);
        ui->statusLabel_AN->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_AN->setText(tr("The entered address does not refer to a key.") + QString(" ") + tr("Please check the address and try again."));
        return;
    }

    WalletModel::UnlockContext ctx(model->requestUnlock());
    if (!ctx.isValid())
    {
        ui->statusLabel_AN->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_AN->setText(tr("Wallet unlock was cancelled."));
        return;
    }

    CKey key;
    if (!pwalletMain->GetKey(keyID, key))
    {
        ui->statusLabel_AN->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_AN->setText(tr("Private key for the entered address is not available."));
        return;
    }

	std::string vrc = ui->addressIn_AN->text().toStdString();
	std::string rpcUser = mapArgs["-rpcuser"];
	std::string rpcPassword = mapArgs["-rpcpassword"];
	std::string rpcPort = mapArgs["-rpcport"];

	QUrl url = QUrl("http://localhost:7876");
	url.addQueryItem(QString("vrc"), QString::fromStdString(vrc)); 
	url.addQueryItem(QString("rpcuser"), QString::fromStdString(rpcUser));
	url.addQueryItem(QString("rpcpassword"), QString::fromStdString(rpcPassword));
	url.addQueryItem(QString("rpcport"), QString::fromStdString(rpcPort));

	QDesktopServices::openUrl(url);
	ui->statusLabel_AN->setStyleSheet("QLabel { color: green; }");
	ui->statusLabel_AN->setText(QString("<nobr>") + tr("Entering SuperNET...") + QString("</nobr>"));
}

void AccessNxtInsideDialog::on_clearButton_AN_clicked()
{
    ui->addressIn_AN->clear();
	ui->statusLabel_AN->clear();
	ui->addressIn_AN->setFocus();
}

bool AccessNxtInsideDialog::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::FocusIn)
    {
        if (ui->tabWidget->currentIndex() == 0)
        {
            /* Clear status message on focus change */
            ui->statusLabel_AN->clear();
        }
    }
    return QDialog::eventFilter(object, event);
}
