#ifndef ATTACKLOG_H
#define ATTACKLOG_H

#include <QDialog>
#include <QProcess>

namespace Ui {
class AttackLog;
}

class AttackLog : public QDialog
{
    Q_OBJECT

public:
    explicit AttackLog(QWidget *parent = 0);
    ~AttackLog();
    void fillSumMaxCorr(QString txt);
    void fillSumMaxKey(QString txt);
    void updateLabelLog(QString txt);

private:
    int getSplitResult(QString txt, QString delim1, QString delim2,
                       QStringList *list1, QStringList *list2);
    Ui::AttackLog *ui;
    int next_byte;
    int is_complete;
};

#endif // ATTACKLOG_H
