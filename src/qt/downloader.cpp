#include "downloader.h"
#include "ui_downloader.h"
#include "bitcoingui.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "util.h"
#include "JlCompress.h"

#include <QLabel>
#include <QProgressBar>
#include <boost/version.hpp>
#include <boost/filesystem.hpp>

using namespace GUIUtil;

Downloader::Downloader(QWidget *parent, WalletModel *walletModel) :
    QDialog(parent),
    walletModel(0),
    ui(new Ui::Downloader)
{
    this->walletModel = walletModel;
    this->setFixedWidth(480);

    ui->setupUi(this);
    ui->urlEdit->setFont(qFont);
    ui->urlEdit->setText("");
    ui->statusLabel->setWordWrap(true);
    ui->statusLabel->setFont(qFont);
    ui->downloadButton->setAutoDefault(false);
    ui->continueButton->setAutoDefault(false);
    ui->quitButton->setAutoDefault(false);

    // Progress bar and label for blockchain download/extract, and auto update
    ui->progressBarLabel->setFont(qFont);
    ui->progressBarLabel->setText(tr("Status:"));
    ui->progressBar->setFont(qFont);
    ui->progressBar->setValue(0);

    // Create a timer to handle hung download requests
    downloadTimer = new QTimer(this);
    connect(downloadTimer, SIGNAL(timeout()), this, SLOT(timerCheckDownloadProgress()));

    // These will be set true when Cancel/Continue/Quit pressed
    downloaderQuit = false;
    httpRequestAborted = false;
    downloadFinished = false;

    // These are set by the class creating the Downloader object
    autoDownload = false;
    processBlockchain = false;
    processUpdate = false;

    // Init these, or else
    reply = 0;
    file = 0;
    manager = 0;

    connect(ui->urlEdit, SIGNAL(textChanged(QString)),
                this, SLOT(enableDownloadButton()));
}

Downloader::~Downloader()
{
    delete ui;
}

void Downloader::showEvent(QShowEvent *e)
{
    if (autoDownload)
    {
        ui->quitButton->setEnabled(true);
        on_downloadButton_clicked();
    }
}

void Downloader::on_continueButton_clicked() // Next button
{
    if (downloadFinished && processBlockchain)
    {
        reloadBlockchain();
    }
    if (downloadFinished && processUpdate)
    {
        checkForUpdate();
    }
    if (downloadFinished && (autoDownload || processBlockchain || processUpdate))
    {
        on_quitButton_clicked();
    }
}

void Downloader::on_quitButton_clicked() // Cancel button
{
    downloaderQuit = true;

    if (!downloadFinished)
    {
        // Clean-up
        if (!httpRequestAborted)
        {
            if (reply)
            {
                reply->abort();
            }
            httpRequestAborted = true;
        }
        downloaderFinished();
    }

    if (processBlockchain)
    {
        BitcoinGUI *p = qobject_cast<BitcoinGUI *>(parent());
        p->reloadBlockchainActionEnabled(true); // Set menu option back to true when dialog closes.
        processBlockchain = false;
        if (!downloadFinished){
            fBootstrapTurbo = false;}
    }
    if (processUpdate)
    {
        BitcoinGUI *p = qobject_cast<BitcoinGUI *>(parent());
        p->checkForUpdateActionEnabled(true); // Set menu option back to true when dialog closes.
        processUpdate = false;
    }

    this->close();
}

void Downloader::closeEvent(QCloseEvent *event)
{
    if (!downloaderQuit)
        on_quitButton_clicked();
    else
        QDialog::closeEvent(event);
}

// Network error ocurred. Download cancelled
void Downloader::networkError()
{
    if (!downloaderQuit)
        cancelDownload();
}

// During the download progress, it can be canceled
void Downloader::cancelDownload()
{
    // Finished with timer
    if (downloadTimer->isActive())
    {
        downloadTimer->stop();
    }

    if (!reply->errorString().isEmpty())
    {
        ui->statusLabel->setText(tr("The download was canceled.\n\n%1").arg(reply->errorString()));
    }
    else
    {
        ui->statusLabel->setText(tr("The download was canceled."));
    }
    if (reply)
    {
        reply->abort();
    }
    httpRequestAborted = true;

    ui->downloadButton->setEnabled(true);
    ui->downloadButton->setDefault(true);
    ui->continueButton->setEnabled(false);
    ui->quitButton->setEnabled(true);
}

void Downloader::on_downloadButton_clicked()
{
    downloadFinished = false;

    // get url
    url = (ui->urlEdit->text());

    QFileInfo fileInfo(url.path());
    QString fileName = fileInfo.fileName();

    if (fileName.isEmpty())
    {
        if (!autoDownload)
        {
            QMessageBox::information(this, tr("Downloader"),
                      tr("Filename cannot be empty.")
                      );
        }
        return;
    }

    if (!fileDest.fileName().isEmpty())
    {
        fileName = fileDest.filePath();
    }
    fileDest = QFileInfo(fileName);

    if (fileDest.exists())
    {
        if (!autoDownload)
        {
            if (QMessageBox::question(this, tr("Downloader"),
                tr("The file \"%1\" already exists. Overwrite it?").arg(fileName),
                QMessageBox::Yes|QMessageBox::No, QMessageBox::No)
                == QMessageBox::No)
            {
                ui->continueButton->setEnabled(true);
                downloadFinished = true;
                ui->progressBar->setMaximum(100);
                ui->progressBar->setValue(100);
                return;
            }
        }
        QFile::remove(fileName);
    }

    manager = new QNetworkAccessManager(this);

    file = new QFile(fileName);
    if (!file->open(QIODevice::WriteOnly))
    {
        if (!autoDownload)
        {
            QMessageBox::information(this, tr("Downloader"),
                      tr("Unable to save the file \"%1\": %2.")
                      .arg(fileName).arg(file->errorString()));
        }
        delete file;
        file = 0;
        ui->continueButton->setEnabled(false);
        return;
    }

    // These will be set true when Cancel/Continue/Quit pressed
    downloaderQuit = false;
    httpRequestAborted = false;

    ui->progressBarLabel->setText(tr("Downloading:"));
    ui->progressBar->setValue(0);

    // download button disabled after requesting download.
    ui->downloadButton->setEnabled(false);
    ui->continueButton->setEnabled(false);

    startRequest(url);
}

// This will be called when download button is clicked (or from Autodownload feature)
void Downloader::startRequest(QUrl url)
{
    downloadProgress = 0;
    downloadFinished = false;

    // Start the timer
    downloadTimer->start(30000);

    // get() method posts a request
    // to obtain the contents of the target request
    // and returns a new QNetworkReply object
    // opened for reading which emits
    // the readyRead() signal whenever new data arrives.
    reply = manager->get(QNetworkRequest(url));
    reply->ignoreSslErrors();

    // Whenever more data is received from the network,
    // this readyRead() signal is emitted
    connect(reply, SIGNAL(readyRead()),
            this, SLOT(httpReadyRead()));

    // Also, downloadProgress() signal is emitted when data is received
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)),
            this, SLOT(updateDownloadProgress(qint64,qint64)));

    // This signal is emitted when the reply has finished processing.
    // After this signal is emitted,
    // there will be no more updates to the reply's data or metadata.
    connect(reply, SIGNAL(finished()),
            this, SLOT(downloaderFinished()));

    // Network error
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(networkError()));

    QString statusText(tr("Please wait for the download to complete..."));
    if (processBlockchain)
    {
        statusText.append(tr("\n\nThe RoxyCoin wallet will restart after extracting the bootstrap%1").arg((fEncrypt ? " and encrypting the wallet." : ".")));
    }
    else if (processUpdate)
    {
        statusText.append(tr("\n\nThe RoxyCoin wallet will restart after downloading the update."));
    }
    ui->statusLabel->setText(statusText);
}

// When download finished or canceled, this will be called
void Downloader::downloaderFinished()
{
    // Finished with timer
    if (downloadTimer->isActive())
    {
        downloadTimer->stop();
    }

    // when canceled
    if (httpRequestAborted)
    {
        if (file)
        {
            file->close();
            file->remove();
            delete file;
            file = 0;
        }
        ui->downloadButton->setEnabled(true);
        ui->downloadButton->setDefault(true);
        ui->continueButton->setEnabled(false);
        ui->quitButton->setEnabled(true);
        return;
    }

    // when partial
    if (processBlockchain && file && file->size() < 1000)
    {
        if (file)
        {
            file->close();
            file->remove();
            delete file;
            file = 0;
        }
        ui->statusLabel->setText("Error: Download ended prematurely.");
        ui->downloadButton->setEnabled(true);
        ui->downloadButton->setDefault(true);
        ui->continueButton->setEnabled(false);
        ui->quitButton->setEnabled(true);
        return;
    }

    // download finished normally
    file->flush();
    file->close();

    // get redirection url
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (reply->error())
    {
        file->remove();
        if (!autoDownload)
        {
            QMessageBox::information(this, tr("Downloader"),
                                 tr("Download terminated: %1.").arg(reply->errorString()));
        }
        ui->downloadButton->setEnabled(true);
        ui->downloadButton->setDefault(true);
        ui->continueButton->setEnabled(false);
    }
    else
    {
        if (!redirectionTarget.isNull())
        {
            QUrl newUrl = url.resolved(redirectionTarget.toUrl());
            if (autoDownload || QMessageBox::question(this, tr("Downloader"),
                                  tr("Redirect to %1 ?").arg(newUrl.toString()),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
            {
                url = newUrl;
                reply->deleteLater();
                file->open(QIODevice::WriteOnly);
                file->resize(0);
                startRequest(url);
                return;
            }
        }
        else
        {
            ui->statusLabel->setText(tr("Download was successful.  Press 'Next' to continue."));
            ui->downloadButton->setEnabled(false);
            ui->continueButton->setEnabled(true);
            ui->continueButton->setDefault(true);
            ui->quitButton->setDefault(false);
        }
    }

    reply->deleteLater();
    reply = 0;
    delete file;
    file = 0;
    manager = 0;
    downloadFinished = true;

    if (autoDownload)
    {
        if (ui->continueButton->isEnabled())
        {
            on_continueButton_clicked();
        }
        else
        {
            on_quitButton_clicked();
        }
    }
}

void Downloader::on_urlEdit_returnPressed()
{
    on_downloadButton_clicked();
}

void Downloader::enableDownloadButton()
{
    ui->downloadButton->setEnabled(!(ui->urlEdit->text()).isEmpty());
}

void Downloader::httpReadyRead()
{
    // this slot gets called every time the QNetworkReply has new data.
    // We read all of its new data and write it into the file.
    // That way we use less RAM than when reading it at the finished()
    // signal of the QNetworkReply
    if (file)
        file->write(reply->readAll());
}

void Downloader::updateDownloadProgress(qint64 bytesRead, qint64 totalBytes)
{
    if (httpRequestAborted)
        return;

    ui->progressBar->setMaximum(totalBytes);
    ui->progressBar->setValue(bytesRead);
}

// This is called during the download to check for a hung state
void Downloader::timerCheckDownloadProgress()
{
    if (ui->progressBar->value() > downloadProgress)
    {
        downloadProgress = ui->progressBar->value();
        return;
    }
    else
    {
        if (!downloadFinished)
        {
            // We appear to be hung.
            cancelDownload();
        }
    }
}

// This is called when the URL is already pre-defined (overloaded)
void Downloader::setUrl(std::string source)
{
    QUrl u;
    u.setUrl(QString::fromStdString(source));
    setUrl(u);
}

// This is called when the URL is already pre-defined
void Downloader::setUrl(QUrl source)
{
    url = source;

    ui->urlEdit->setText(url.url());
    ui->urlEdit->setEnabled(false);
}

// This is called when the Destination is already pre-defined (overloaded)
void Downloader::setDest(std::string dest)
{
    QString d = QString::fromStdString(dest);
    setDest(d);
}

// This is called when the Destination is already pre-defined
void Downloader::setDest(QString dest)
{
    fileDest = QFileInfo(dest);

    if (fileDest.exists())
    {
        ui->statusLabel->setText(tr("The file \"%1\" already exists.\n\nPress 'Next' to continue with this file, or 'Download' to get a new one.").arg(fileDest.filePath()));
        ui->continueButton->setEnabled(true);
        ui->progressBar->setMaximum(100);
        ui->progressBar->setValue(100);
        downloadFinished = true;
    }
    else
    {
        ui->statusLabel->setText(tr("Press 'Download' or 'Next' to begin."));
        ui->continueButton->setEnabled(false);
    }
}

void Downloader::reloadBlockchain()
{
    ui->statusLabel->setText(tr("Please wait...."));
    ui->downloadButton->setEnabled(false);
    ui->continueButton->setEnabled(false);
    ui->quitButton->setEnabled(false);
    ui->downloadButton->setDefault(false);
    ui->continueButton->setDefault(false);
    ui->quitButton->setDefault(true);
    this->raise();

    ui->progressBarLabel->setText(tr("Extracting:"));
    ui->progressBar->setValue(0);

    if (boost::filesystem::exists(fileDest.filePath().toStdString()))
    {
        printf("Preparing for Blockchain Reload...\n");
    }
    else
    {
        printf("Download failed!\n");
        ui->statusLabel->setText(tr("An error ocurred. The bootstrap file could not be found. Please try to download it again."));
        ui->downloadButton->setEnabled(true);
        ui->continueButton->setEnabled(false);
        ui->quitButton->setEnabled(true);
        ui->downloadButton->setDefault(true);
        ui->continueButton->setDefault(false);
        ui->quitButton->setDefault(false);
        downloadFinished = false;
        return;
    }

    // Test the archive.
    QStringList zlist = JlCompress::getFileList(fileDest.filePath(), 1);
    if (!zlist.isEmpty() && zlist[0].contains("bootstrap/"))
    {
        printf("Bootstrap structure is valid.\n");
    }
    else
    {
        printf("Bootstrap structure is invalid!\n");
        ui->statusLabel->setText(tr("I'm sorry, the bootstrap file structure appears to be invalid. Please try to download it again."));
        ui->downloadButton->setEnabled(true);
        ui->continueButton->setEnabled(false);
        ui->quitButton->setEnabled(true);
        ui->downloadButton->setDefault(false);
        ui->continueButton->setDefault(false);
        ui->quitButton->setDefault(true);
        downloadFinished = false;
        return;
    }

    // Extract bootstrap.zip
    QStringList zextracted = JlCompress::extractDir(fileDest.filePath(), fileDest.path(), ui->progressBar);

    if (!zextracted.isEmpty())
    {
        printf("Bootstrap extract successful.\n");
    }
    else
    {
        printf("Bootstrap extract failed!\n");
        ui->statusLabel->setText(tr("I'm sorry, the bootstrap extract failed."));
        ui->downloadButton->setEnabled(true);
        ui->continueButton->setEnabled(false);
        ui->quitButton->setEnabled(true);
        ui->downloadButton->setDefault(false);
        ui->continueButton->setDefault(false);
        ui->quitButton->setDefault(true);
        downloadFinished = false;
        return;
    }

    if (!boost::filesystem::exists(GetDataDir() / "bootstrap" / "blk0001.dat") ||
        !boost::filesystem::exists(GetDataDir() / "bootstrap" / "txleveldb"))
    {
        printf("Bootstrap extract is invalid!\n");
        ui->statusLabel->setText(tr("I'm sorry, the bootstrap extract was successful, but the contents are invalid."));
        ui->downloadButton->setEnabled(true);
        ui->continueButton->setEnabled(false);
        ui->quitButton->setEnabled(true);
        ui->downloadButton->setDefault(false);
        ui->continueButton->setDefault(false);
        ui->quitButton->setDefault(true);
        downloadFinished = false;
        return;
    }

    ui->progressBarLabel->setText(tr("Complete:"));
    ui->statusLabel->setText(tr("Congratulations, the bootstrap extract was successful! Your wallet will restart %1.").arg(fEncrypt ? "after encrypting the wallet" : "to complete the operation"));
    ui->downloadButton->setEnabled(false);
    ui->continueButton->setEnabled(false);
    ui->quitButton->setEnabled(false);
    this->raise();
    this->repaint();
    MilliSleep(5 * 1000);

    // If the wallet is still encrypting, hold off on the restart
    if (this->walletModel && !fEncrypt)
    {
        if (!walletModel->reloadBlockchain())
        {
            fBootstrapTurbo = false;
            QMessageBox::warning(this, tr("Reload Failed"), tr("There was an error trying to reload the blockchain."));
        }
    }
}

void Downloader::checkForUpdate()
{
    this->raise();

    ui->statusLabel->setText(tr("Congratulations! Your wallet will now restart..."));
    ui->downloadButton->setEnabled(false);
    ui->continueButton->setEnabled(false);
    ui->quitButton->setEnabled(false);
    ui->downloadButton->setDefault(false);
    ui->continueButton->setDefault(false);
    ui->quitButton->setDefault(false);

    MilliSleep(3000);

    // Restart with the executable.
    if (this->walletModel)
    {
        if (!walletModel->checkForUpdate())
        {
            QMessageBox::warning(this, tr("Update Failed"), tr("There was an error trying to update the wallet."));
        }
    }
}

void Downloader::on_confCheckBox_clicked(bool checked)
{
    if (checked)
    {
        fBootstrapConfig = true;
    }
    else
    {
        fBootstrapConfig = false;
    }
}
