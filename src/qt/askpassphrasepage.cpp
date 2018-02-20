#include "askpassphrasepage.h"
#include "ui_askpassphrasepage.h"
#include "init.h"
#include "util.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "walletmodel.h"
#include "bitcoingui.h"

#include <QPushButton>
#include <QKeyEvent>

using namespace GUIUtil;

AskPassphrasePage::AskPassphrasePage(Mode mode, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AskPassphrasePage),
    mode(mode),
    model(0),
    fCapsLock(false)
{
    ui->setupUi(this);
    this->layout()->setContentsMargins(0, 0, 0, 0);
    this->setStyleSheet(GUIUtil::veriAskPassphrasePageStyleSheet);

    ui->formLayout->setContentsMargins(220, 320 + HEADER_HEIGHT, 220, 0);
    ui->messageLabel->setFont(qFontBold);
    ui->passEdit1->setMaxLength(MAX_PASSPHRASE_SIZE);
    ui->passEdit2->setMaxLength(MAX_PASSPHRASE_SIZE);
    ui->passEdit3->setMaxLength(MAX_PASSPHRASE_SIZE);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    QWidget::setTabOrder(ui->passEdit1, ui->passEdit2);
    QWidget::setTabOrder(ui->passEdit2, ui->passEdit3);
    QWidget::setTabOrder(ui->passEdit3, ui->buttonBox->button(QDialogButtonBox::Ok));

    // Setup Caps Lock detection.
    ui->passEdit1->installEventFilter(this);
    ui->passEdit2->installEventFilter(this);
    ui->passEdit3->installEventFilter(this);

    switch(mode)
    {
        case Encrypt: // Ask passphrase x2
            ui->passEdit1->hide();
            ui->passEdit2->show();
            ui->passEdit3->show();
            ui->passEdit2->setFocus();
            ui->passEdit2->setPlaceholderText(tr("New passphrase..."));
            ui->passEdit3->setPlaceholderText(tr("Repeat new passphrase..."));
            ui->messageLabel->setText(tr("Welcome! Please enter a passphrase of 10 or more characters, or 8 or more words."));
            ui->warningLabel->setText(tr("WARNING: IF YOU LOSE YOUR PASSPHRASE, YOU WILL LOSE ALL OF YOUR COINS!"));
            break;
        case Lock: // Ask passphrase
            ui->passEdit1->hide();
            ui->passEdit2->hide();
            ui->passEdit3->hide();
            ui->messageLabel->setText(tr("Click OK to lock the wallet."));
            break;
        case Unlock: // Ask passphrase
            ui->passEdit1->show();
            ui->passEdit2->hide();
            ui->passEdit3->hide();
            ui->passEdit1->setFocus();
            ui->passEdit1->setPlaceholderText(tr("Enter passphrase..."));
            ui->messageLabel->setText(tr("Please enter your passphrase to unlock the wallet."));
            break;
    }

    connect(ui->passEdit1, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));
    connect(ui->passEdit2, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));
    connect(ui->passEdit3, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));

    connect(ui->passEdit1, SIGNAL(returnPressed()), this, SLOT(accept()));
    connect(ui->passEdit2, SIGNAL(returnPressed()), this, SLOT(accept()));
    connect(ui->passEdit3, SIGNAL(returnPressed()), this, SLOT(accept()));
    connect(ui->buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
}

AskPassphrasePage::~AskPassphrasePage()
{
    // Attempt to overwrite text so that they do not linger around in memory
    ui->passEdit1->setText(QString(" ").repeated(ui->passEdit1->text().size()));
    ui->passEdit2->setText(QString(" ").repeated(ui->passEdit2->text().size()));
    ui->passEdit3->setText(QString(" ").repeated(ui->passEdit3->text().size()));
    delete ui;
}

void AskPassphrasePage::setModel(WalletModel *model)
{
    this->model = model;
}

void AskPassphrasePage::accept()
{
    if(!model)
        return;

    SecureString oldpass, newpass1, newpass2;
    oldpass.reserve(MAX_PASSPHRASE_SIZE);
    newpass1.reserve(MAX_PASSPHRASE_SIZE);
    newpass2.reserve(MAX_PASSPHRASE_SIZE);
    // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make this input mlock()'d to begin with.
    oldpass.assign(ui->passEdit1->text().toStdString().c_str());
    newpass1.assign(ui->passEdit2->text().toStdString().c_str());
    newpass2.assign(ui->passEdit3->text().toStdString().c_str());

    switch(mode)
    {
    case Encrypt: // Encrypt wallet and set password
        if(newpass1.empty() || newpass2.empty())
        {
            ui->messageLabel->setText(tr("The new passphrase cannot be blank."));
            break;
        }
        if(newpass1.length() < 10)
        {
            ui->messageLabel->setText(tr("The new passphrase must be 10 or more characters."));
            break;
        }
        if(newpass1 != newpass2)
        {
            ui->messageLabel->setText(tr("The passphrases do not match."));
            break;
        }
        else
        {
            if(model->setWalletEncrypted(true, newpass1))
            {
                ui->messageLabel->setText(tr("RoxyCoin will now restart to finish the encryption process."));
                ui->warningLabel->setText(tr("IMPORTANT: Previous backups of your wallet should be replaced with the new one."));
                ui->passEdit2->setText(QString(" ").repeated(ui->passEdit2->text().size()));
                ui->passEdit2->setText(QString(""));
                ui->passEdit2->setEnabled(false);
                ui->passEdit3->setText(QString(" ").repeated(ui->passEdit3->text().size()));
                ui->passEdit3->setText(QString(""));
                ui->passEdit3->setEnabled(false);
                ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
                this->repaint();

                fRestart = true;
                MilliSleep(7 * 1000);
                StartShutdown();
            }
            else
            {
                ui->messageLabel->setText(tr("Wallet encryption failed due to an internal error. Your wallet was not encrypted."));
            }
        }
        break;
    case Lock: // Turn off staking
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        emit lockWalletFeatures(true);
        break;
    case Unlock: // Turn on staking
        if(!model->setWalletLocked(false, oldpass))
        {
            ui->messageLabel->setText(tr("The passphrase entered for the wallet is incorrect."));
        }
        else
        {
            // Attempt to overwrite text so that they do not linger around in memory
            ui->passEdit1->setText(QString(" ").repeated(ui->passEdit1->text().size()));
            oldpass.assign(ui->passEdit1->text().toStdString().c_str());
            ui->passEdit1->setText(QString(""));
            ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            emit lockWalletFeatures(false);
        }
        break;
    }
}

void AskPassphrasePage::textChanged()
{
    // Validate input, set Ok button to enabled when acceptable
    bool acceptable = true;
    switch(mode)
    {
    case Lock: // Old passphrase x1
        break;
    case Unlock: // Old passphrase x1
        break;
    }
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(acceptable);
}

bool AskPassphrasePage::event(QEvent *event)
{
    // Detect Caps Lock key press.
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_CapsLock) {
            fCapsLock = !fCapsLock;
        }
        if (fCapsLock) {
            ui->warningLabel->setText(tr("WARNING: The Caps Lock key is on!"));
        } else {
            ui->warningLabel->clear();
        }
    }
    return QWidget::event(event);
}

bool AskPassphrasePage::eventFilter(QObject *object, QEvent *event)
{
    /* Detect Caps Lock.
     * There is no good OS-independent way to check a key state in Qt, but we
     * can detect Caps Lock by checking for the following condition:
     * Shift key is down and the result is a lower case character, or
     * Shift key is not down and the result is an upper case character.
     */
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        QString str = ke->text();
        if (str.length() != 0) {
            const QChar *psz = str.unicode();
            bool fShift = (ke->modifiers() & Qt::ShiftModifier) != 0;
            if ((fShift && psz->isLower()) || (!fShift && psz->isUpper())) {
                fCapsLock = true;
                ui->warningLabel->setText(tr("WARNING: The Caps Lock key is on!"));
            } else if (psz->isLetter()) {
                fCapsLock = false;
                ui->warningLabel->clear();
            }
        }
    }
    return QWidget::eventFilter(object, event);
}
