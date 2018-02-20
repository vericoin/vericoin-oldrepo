#include "updatedialog.h"
#include "ui_updatedialog.h"
#include "clientmodel.h"
#include "util.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "version.h"

using namespace GUIUtil;

bool updateAccepted = false;

UpdateDialog::UpdateDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UpdateDialog)
{
    std::string title = GetArg("-vTitle", "RoxyCoin Update Available");
    std::string description = GetArg("-vDescription", "Lot's of new features.").c_str();
    std::string version = "NEW IN " + GetArg("-vVersion", "1.0.0.0");
    std::string postreq = std::string("\nPost-Install Notes: ").append((GetBoolArg("-vBootstrap") ? "Auto Bootstrap will run after the installation." : "Bootstrapping is not required."));
    ui->setupUi(this);

    ui->title->setFont(qFontLarge);
    ui->title->setText(title.c_str());
    ui->description->setFont(qFont);
    ui->description->setText(version.append(": ").append(description).append(postreq).append("\n\nPress OK to download the update.").c_str());
}

void UpdateDialog::setModel(ClientModel *model)
{
    if(model)
    {
        ui->versionLabel->setText(model->formatFullVersion().append(GetArg("-vArch", "").c_str()).append("  (Update Available)"));
    }
}

UpdateDialog::~UpdateDialog()
{
    delete ui;
}

void UpdateDialog::on_buttonBox_accepted()
{
    updateAccepted = true;
    close();
}

void UpdateDialog::on_buttonBox_rejected()
{
    updateAccepted = false;
    close();
}
