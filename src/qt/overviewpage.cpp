#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "util.h"
#include "init.h"
#include "walletmodel.h"
#include "clientmodel.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "transactiontablemodel.h"
#include "transactionfilterproxy.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "bitcoingui.h"
#include "bitcoinrpc.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QDesktopServices>
#include <QUrl>
#include <QGraphicsView>

using namespace GUIUtil;

#define DECORATION_SIZE 46
#define NUM_ITEMS 3

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(): QAbstractItemDelegate(), unit(BitcoinUnits::VRC)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(qVariantCanConvert<QColor>(value))
        {
            foreground = COLOR_BAREADDRESS;
        }

        painter->setPen(foreground);
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address);

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if (amount > 0)
        {
            foreground = COLOR_POSITIVE;
        }
        else
        {
            foreground = COLOR_BAREADDRESS;
        }
        painter->setPen(foreground);
        QString amountText =  BitcoinUnits::formatMaxDecimals(unit, amount, decimals, true, hideAmounts);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;
    int decimals;
    bool hideAmounts;

};
#include "overviewpage.moc"

OverviewPage::OverviewPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    currentBalance(-1),
    currentUnconfirmedBalance(-1),
    currentStakeBalance(-1),
    txdelegate(new TxViewDelegate()),
    filter(0),
    Staking(GetBoolArg("-staking",true))
{
    // Setup header and styles
    if (fNoHeaders)
        GUIUtil::header(this, QString(""));
    else
        GUIUtil::header(this, QString(":images/headerOverview"));

    ui->setupUi(this);
    this->layout()->setContentsMargins(0, 0 + HEADER_HEIGHT, 0, 0);

    // try to make more room for small screens
    if (fNoHeaders) {
        ui->formLayout_4->layout()->setContentsMargins(10, 10, 0, 0);
        ui->gridLayout_3->layout()->setContentsMargins(0, 10, 10, 0);
        ui->gridLayout_4->layout()->setContentsMargins(0, 0, 10, 0);
        ui->formLayout_6->layout()->setContentsMargins(10, 0, 0, 0);
    }

    ui->labelBalance->setFont(qFontLargerBold);
    ui->labelTransactions->setFont(qFontLargerBold);
    ui->labelNetwork->setFont(qFontLargerBold);
    ui->labelValue->setFont(qFontLargerBold);

    ui->labelSpendableText->setFont(qFont);
    ui->labelSpendable->setFont(qFont);
    ui->labelStakeText->setFont(qFont);
    ui->labelStake->setFont(qFont);
    ui->labelUnconfirmedText->setFont(qFont);
    ui->labelUnconfirmed->setFont(qFont);
    ui->labelTotalText->setFont(qFont);
    ui->labelTotal->setFont(qFont);

    // staker section
    ui->stakingLabel->setFont(qFont);

    //statistics section
    ui->interestRateText->setFont(qFont);
    ui->interestRate->setFont(qFont);
    ui->inflationRateText->setFont(qFont);
    ui->inflationRate->setFont(qFont);
    ui->netstakeweightText->setFont(qFont);
    ui->netstakeweight->setFont(qFont);
    ui->stakeRateText->setFont(qFont);
    ui->stakeRate->setFont(qFont);
    ui->blocknumberText->setFont(qFont);
    ui->blocknumber->setFont(qFont);

    // Add icons to the Balance section
    ui->labelSpendableText->setText("<html><img src=':icons/spendable' width=16 height=16 border=0 align='bottom'> Spendable:</html>");
    ui->labelStakeText->setText("<html><img src=':icons/stakingoff' width=16 height=16 border=0 align='bottom'> Staking Coins:</html>");
    ui->labelUnconfirmedText->setText("<html><img src=':icons/unconfirmed' width=16 height=16 border=0 align='bottom'> Unconfirmed:</html>");
    ui->labelTotalText->setText("<html><img src=':icons/total' width=16 height=16 border=0 align='bottom'> Total:</html>");

    // Recent transactionsBalances
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 1));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->listTransactions->setMouseTracking(true);
    ui->listTransactions->viewport()->setAttribute(Qt::WA_Hover, true);
    ui->listTransactions->setStyleSheet("QListView { background: white; color: " + STR_FONT_COLOR + "; border-radius: 10px; border: 0; padding-right: 10px; padding-bottom: 5px; } \
                                         QListView::hover { background: qlineargradient(x1: 0, y1: 1, x2: 0, y2: 0, stop: 0 #fafbfe, stop: 1 #ECF3FA); }");
    ui->listTransactions->setFont(qFont);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of date") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of date") + ")");

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);

    // set initial state of mining button
    if (Staking)
    {
        ui->stakeButton->setIcon(QIcon(":/icons/stakingon"));
        ui->stakingLabel->setText("Click to stop:");
    }
    else{
        ui->stakeButton->setIcon(QIcon(":/icons/stakingoff"));
        ui->stakingLabel->setText("Click to start:");
    }
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        emit transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setBalance(qint64 balance, qint64 stake, qint64 unconfirmedBalance)
{
    QString maxDecimalsTooltipText("\nUse Settings/Options/Display to hide decimals.");
    qint64 total = balance + stake + unconfirmedBalance;

    BitcoinUnits *bcu = new BitcoinUnits(this, this->model);
    int unit = model->getOptionsModel()->getDisplayUnit();
    bool hideAmounts = model->getOptionsModel()->getHideAmounts();
    if (model->getOptionsModel()->getDecimalPoints() < bcu->maxdecimals(unit))
    {
        maxDecimalsTooltipText = QString(""); // The user already knows about the option to set decimals
    }

    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentStakeBalance = stake;

    ui->labelSpendable->setText(bcu->formatWithUnit(unit, balance, false, hideAmounts));
    ui->labelSpendable->setToolTip(tr("%1%2").arg(bcu->formatWithUnitWithMaxDecimals(unit, balance, bcu->maxdecimals(unit), true, false)).arg(maxDecimalsTooltipText));
    ui->labelUnconfirmed->setText(bcu->formatWithUnit(unit, unconfirmedBalance, false, hideAmounts));
    ui->labelUnconfirmed->setToolTip(tr("%1%2").arg(bcu->formatWithUnitWithMaxDecimals(unit, unconfirmedBalance, bcu->maxdecimals(unit), true, false)).arg(maxDecimalsTooltipText));
    ui->labelStake->setText(bcu->formatWithUnit(unit, stake, false, hideAmounts));
    ui->labelStake->setToolTip(tr("%1%2").arg(bcu->formatWithUnitWithMaxDecimals(unit, stake, bcu->maxdecimals(unit), true, false)).arg(maxDecimalsTooltipText));
    ui->labelTotal->setText(bcu->formatWithUnit(unit, total, false, hideAmounts));
    ui->labelTotal->setToolTip(tr("%1%2").arg(bcu->formatWithUnitWithMaxDecimals(unit, total, bcu->maxdecimals(unit), true, false)).arg(maxDecimalsTooltipText));

    delete bcu;
}

void OverviewPage::setStatistics()
{
    double nNetworkWeight = boost::rational_cast<double>(GetPoSKernelPS());
    int stakerate = 0;
    if (Staking){
        uint64_t timetillstake = GetTimeToStake();
        if (timetillstake > 3600){
            stakerate = timetillstake/(60*60);
        }
        else if (timetillstake == 0){
            stakerate = 0;
        }
        else{
            stakerate = 1;
        }
    }
    // display stats
    ui->blocknumber->setText(QString::number(pindexBest->nHeight));
    ui->netstakeweight->setText(QString::number((double)((nNetworkWeight/2)/30000000)*100,'f',2));
    ui->stakeRate->setText(QString::number(((int)stakerate)));
    ui->inflationRate->setText(QString::number(boost::rational_cast<double>(GetCurrentInflationRate(nNetworkWeight)),'f',2));
    ui->interestRate->setText(QString::number(boost::rational_cast<double>(GetCurrentInterestRate(pindexBest)),'f',2));
}

void OverviewPage::setModel(WalletModel *model)
{
    this->model = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getStake(), model->getUnconfirmedBalance());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64)));
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        connect(model->getOptionsModel(), SIGNAL(decimalPointsChanged(int)), this, SLOT(updateDecimalPoints()));
        connect(model->getOptionsModel(), SIGNAL(hideAmountsChanged(bool)), this, SLOT(updateHideAmounts()));
    }

    // update the display unit, to not use the default ("VRC")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, currentStakeBalance, currentUnconfirmedBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = model->getOptionsModel()->getDisplayUnit();
        txdelegate->decimals = model->getOptionsModel()->getDecimalPoints();
        txdelegate->hideAmounts = model->getOptionsModel()->getHideAmounts();

        ui->listTransactions->update();
    }
}

void OverviewPage::updateDecimalPoints()
{
    updateDisplayUnit();
}

void OverviewPage::updateHideAmounts()
{
    updateDisplayUnit();
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}

void OverviewPage::myOpenUrl(QUrl url)
{
    QDesktopServices::openUrl(url);
}

void OverviewPage::sslErrorHandler(QNetworkReply* qnr, const QList<QSslError> & errlist)
{
    qnr->ignoreSslErrors();
}

void OverviewPage::on_stakeButton_clicked()
{
    ui->stakeButton->clearFocus();
    if (!Staking)
    {
        model->requestUnlock();
        if (model->getEncryptionStatus() == WalletModel::Unlocked)
        {
            startStaking();
        }
    }
    else
    {
        model->requestLock();
        if (model->getEncryptionStatus() == WalletModel::Locked)
        {
            stopStaking();
        }
    }
}

void OverviewPage::startStaking()
{
        Staking = true;
        ui->stakingLabel->setText("Click to stop:");
        ui->stakeButton->setIcon(QIcon(":/icons/stakingon"));
        MilliSleep(100);
        setStatistics();
}

void OverviewPage::stopStaking()
{
        Staking = false;
        ui->stakingLabel->setText("Click to start:");
        ui->stakeButton->setIcon(QIcon(":/icons/stakingoff"));
        MilliSleep(100);
        setStatistics();
}
