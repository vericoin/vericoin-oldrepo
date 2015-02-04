#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "clientmodel.h"
#include "util.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "version.h"

using namespace GUIUtil;

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    this->setStyleSheet(GUIUtil::veriStyleSheet);
}

void AboutDialog::setModel(ClientModel *model)
{
    if(model)
    {
        ui->versionLabel->setText(model->formatFullVersion().append(" ").append(GetArg("-vArch", "").c_str()));
    }
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::on_buttonBox_accepted()
{
    close();
}
