#include "whatsnewdialog.h"
#include "ui_whatsnewdialog.h"
#include "clientmodel.h"
#include "util.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "version.h"

using namespace GUIUtil;

bool whatsNewAccepted = false;

WhatsNewDialog::WhatsNewDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WhatsNewDialog)
{
    std::string title = "What's New in RoxyCoin";
    std::string description = GetArg("-vDescription", "Error downloading version data. Please try again later.").c_str();
    std::string version = "NEW IN " + GetArg("-vVersion", "0.0");
    ui->setupUi(this);

    ui->title->setFont(qFontLarge);
    ui->title->setText(title.c_str());
    ui->description->setFont(qFont);
    ui->description->setText(version.append(": ").append(description).c_str());
}

void WhatsNewDialog::setModel(ClientModel *model)
{
    if(model)
    {
        ui->versionLabel->setText(model->formatFullVersion().append(GetArg("-vArch", "").c_str()).append("  (No Update Available)"));
    }
}

WhatsNewDialog::~WhatsNewDialog()
{
    delete ui;
}

void WhatsNewDialog::on_buttonBox_accepted()
{
    whatsNewAccepted = true;
    close();
}
