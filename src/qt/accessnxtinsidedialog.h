#ifndef ACCESSNXTINSIDEDIALOG_H
#define ACCESSNXTINSIDEDIALOG_H

#include <QDialog>

namespace Ui {
    class AccessNxtInsideDialog;
}
class WalletModel;

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

class AccessNxtInsideDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AccessNxtInsideDialog(QWidget *parent = 0);
    ~AccessNxtInsideDialog();

    void setModel(WalletModel *model);
    void setAddress_AN(QString address);
    void showTab_AN(bool fShow);

protected:
    bool eventFilter(QObject *object, QEvent *event);

private:
    Ui::AccessNxtInsideDialog *ui;
    WalletModel *model;

private slots:
    /* Access Nxt */
    void on_addressBookButton_AN_clicked();
    void on_pasteButton_AN_clicked();
	void on_accessNxtInsideButton_AN_clicked();
    void on_clearButton_AN_clicked();
};

#endif // ACCESSNXTINSIDEDIALOG_H
