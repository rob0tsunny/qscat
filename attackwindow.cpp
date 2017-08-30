#include "attackwindow.h"
#include "ui_attackwindow.h"
#include "scatool.h"
#include <assert.h>
#include <QFileDialog>
#include <QSpinBox>
#include <QTableWidget>
#include <QByteArray>
#include <qdebug.h>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QMessageBox>
#include <QTemporaryDir>
#include <QProcess>
#include <QStringList>
#include <QMovie>
#include <QLabel>
#include <QHeaderView>
#include <QDragEnterEvent>
#include <QMimeData>

Attackwindow::Attackwindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Attackwindow)
{
    QString daredevil_local = QDir::currentPath();

    ui->setupUi(this);

    int algo_idx = 0;
    algo_map["Message"] = algo_idx++;
    algo_map["AES"] = algo_idx++;
    algo_map["DES"] = algo_idx++;

    ui->algoBox->clear();
    ui->algoBox->addItems(QStringList()
                          <<"Identity"
                          <<"AES"
                          <<"DES");

    ui->functionBox->clear();
    ui->functionBox->addItems(QStringList()
                              <<"Input"
                              <<"1st round output SBOX"
                              <<"1st XOR");

    ui->methodBox->clear();
    ui->methodBox->addItems(QStringList() << "CPA");

    ui->byteIdxTable->resizeColumnsToContents();
    ui->byteIdxTable->resizeRowsToContents();
    ui->byteIdxTable->hasAutoScroll();

    ui->spinnb_traces->setMaximum(0);

    QHeaderView* headerh = ui->byteIdxTable->horizontalHeader();
    headerh->setSectionResizeMode(QHeaderView::Stretch);

    QHeaderView* headerw = ui->byteIdxTable->verticalHeader();
    headerw->setSectionResizeMode(QHeaderView::Stretch);

    this->daredevil_path.clear();
    if (!system(daredevil_local.toUtf8() + "/daredevil/daredevil -h"))
        /* exec from qscat folder */
        this->daredevil_path.append(daredevil_local.toUtf8() + "/daredevil/");
    else if (!system(daredevil_local.toUtf8() + "/../qscat/daredevil/daredevil -h"))
        /* exec from dev folder */
        this->daredevil_path.append(daredevil_local.toUtf8() + "/../qscat/daredevil/");
}

Attackwindow::~Attackwindow()
{
    delete ui;
}

void Attackwindow::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls())
    {
        QList<QUrl> urls = mimeData->urls();
        if (urls.size() == 1)
        {
            QUrl url = urls.at(0);
            if (url.isValid() && (url.scheme().toLower() == "file") && mimeData->hasFormat("text/uri-list"))
            {
                event->acceptProposedAction();
            }
        }
    }
}

void Attackwindow::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls())
    {
        QList<QUrl> urlList = mimeData->urls();
        load_dataSet(urlList.at(0).toLocalFile());
    }
}

void Attackwindow::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void Attackwindow::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}

void Attackwindow::load_dataSet(QString filepath_dataset)
{
    int input_len;
    this->input_dataset = filepath_dataset;
    this->qf.setFileName(this->input_dataset);
    if (!this->qf.open(QIODevice::ReadOnly))
        return;

    ui->tableWidget->clear();
    ui->tableWidget->setRowCount(0);

    switch (ui->algoBox->currentIndex()) {
    case 0:
        input_len = 16;
        break;
    case 1:
        input_len = 16;
        break;
    case 2:
        input_len = 8;
        break;
    default:
        input_len = -1;
    }

    if (input_len < 0)
        return;

    QByteArray bin = this->qf.readAll();
    if (bin.length() % input_len)
        return;

    int row_index = 0;
    for (int i = 0; i < bin.length(); i+=16) {
        QString cl = "";
        for (int j = 0; j < input_len; j++)
            cl.append(QString().sprintf("%02x",(unsigned char)bin[i+j]));
        cl.append("\n");
        ui->tableWidget->insertRow(row_index);
        ui->tableWidget->setItem(row_index, 0, new QTableWidgetItem(cl));
        row_index++;
    }

    ui->spinnb_traces->setMaximum(row_index);
    ui->spinnb_traces->setValue(row_index);
}

void Attackwindow::on_DataButton_pressed()
{
    QString filepath_dataset;
    filepath_dataset = QFileDialog::getOpenFileName(this, QString("Select data set file"));
    load_dataSet(filepath_dataset);
}

void Attackwindow::on_spinpts_start_valueChanged(int arg1)
{
    //Update value range
    if (ScaTool::curves->length() > 0) {
        ui->spinpts_start->setRange(0,ScaTool::curves->first()->length());
        ui->spinpts_end->setRange(0,ScaTool::curves->first()->length());
    }

    // Respect min and max
    if (arg1 > ui->spinpts_end->value())
        ui->spinpts_end->setValue(ui->spinpts_start->value());
}

void Attackwindow::on_spinpts_end_valueChanged(int arg1)
{
    //Update value range
    if (ScaTool::curves->length() > 0) {
        ui->spinpts_start->setRange(0,ScaTool::curves->first()->length());
        ui->spinpts_end->setRange(0,ScaTool::curves->first()->length());
    }

    // Respect min and max
    if (ui->spinpts_start->value() > arg1)
        ui->spinpts_start->setValue(ui->spinpts_end->value());
}

void Attackwindow::on_ClearButton_pressed()
{
    ui->tableWidget->clear();
    ui->tableWidget->setRowCount(0);
    ui->spinnb_traces->setMaximum(0);
    ui->spinnb_traces->setValue(0);
}

void Attackwindow::on_attackButton_pressed()
{
    // Get values from UI
    int attack_method = ui->methodBox->currentIndex();
    int algo = ui->algoBox->currentIndex();
    int sel_fun = ui->functionBox->currentIndex();
    int pts_min = ui->spinpts_start->value();
    int pts_max = ui->spinpts_end->value();
    int nb_pts = pts_max - pts_min;
    int nb_traces = ui->spinnb_traces->value();
    //ui->byteIdxTable->selectedItems()
    QMovie *movie = new QMovie(":images/ajax-loader.gif");
    QLabel *pr = new QLabel(this);
    pr->setMovie(movie);
    pr->show();
    movie->start();
    qApp->processEvents();

    // Create a working directory
    QTemporaryDir tdir;
    assert(tdir.isValid());

    // Create traces data file
    QFile trace(tdir.path() + "/trace.bin");
    assert(trace.open(QIODevice::ReadWrite));
    for (int i = 0; i < nb_traces; i++)
    {
        Curve* c = ScaTool::curves->at(i);
        //float * buf = c->getrawdata(&nb_pts,pts_min - c->xoffset);
        float * buf = c->getrawdata(&nb_pts, c->xoffset);
        trace.write(reinterpret_cast<const char*>(buf), nb_pts<<2);
        free(buf);

    }

    QFile::copy(this->input_dataset, tdir.path() + "/input.bin");
    QFile::copy(this->daredevil_path + "LUT/AES_AFTER_SBOX",
                tdir.path() + "/lut");

    // Create config file
    QFile config(tdir.path() + "/CONFIG");
    assert(config.open(QIODevice::ReadWrite | QIODevice::Text));
    config.write("[Traces]\n");
    config.write("files=1\n");
    config.write("trace_type=f\n");
    config.write("transpose=false\n");
    config.write("index=0\n");
    config.write("nsamples=" + QString::number(nb_pts).toUtf8() + "\n");
    config.write("trace=" + tdir.path().toUtf8() + "/trace.bin "
                 + QString::number(nb_traces).toUtf8() + " "
                 + QString::number(nb_pts).toUtf8() + "\n");

    config.write("[Guesses]\n");
    config.write("files=1\n");
    config.write("guess_type=u\n");
    config.write("transpose=false\n");
    config.write("guess=" + tdir.path().toUtf8() +"/input.bin" + " "
                 + QString::number(nb_traces).toUtf8() + " 16\n");
    config.write("[General]\n");
    config.write("threads=8\n");
    config.write("order=1\n");
    config.write("return_type=double\n");
    config.write("algorithm=AES\n");
    config.write("position=" + tdir.path().toUtf8() + "/lut\n");
    config.write("round=0\n");
    config.write("bytenum=all\n");
    config.write("bitnum=none\n");
    config.write("memory=4G\n");
    config.write("top=20\n");

    config.flush();
    trace.flush();
    config.close();
    trace.close();

    // Launch daredevil
    QProcess daredevil;
    daredevil.start(this->daredevil_path + "daredevil -c " + config.fileName(),
                    QIODevice::ReadOnly);
    daredevil.waitForFinished();

    ScaTool::attacklog->showlog(daredevil.readAllStandardOutput());

    trace.remove();
    tdir.remove();
    movie->stop();
    pr->hide();
    qApp->processEvents();
}
