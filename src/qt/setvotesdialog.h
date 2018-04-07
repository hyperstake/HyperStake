#ifndef HYPERSTAKE_SETVOTESDIALOG_H
#define HYPERSTAKE_SETVOTESDIALOG_H

#include <QDialog>
#include <QObject>

class WalletModel;

namespace Ui {
    class SetVotesDialog;
}

class SetVotesDialog : public QDialog
{
    Q_OBJECT
public:
    SetVotesDialog(QWidget* parent);
    ~SetVotesDialog();
    void SetWalletModel(WalletModel* model);
    void UpdateTable();
private slots:
    void on_voteButton_clicked();
private:
    Ui::SetVotesDialog* ui;
    WalletModel* walletModel;
    void Clear();
};

#endif //HYPERSTAKE_SETVOTESDIALOG_H
