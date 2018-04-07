#include "proposalsdialog.h"
#include "ui_proposalsdialog.h"

#include "db.h"
#include "main.h"
#include "walletmodel.h"
#include <QStandardItemModel>


ProposalsDialog::ProposalsDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::ProposalsDialog)
{
    ui->setupUi(this);
}

ProposalsDialog::~ProposalsDialog()
{
    delete ui;
}

void ProposalsDialog::SetWalletModel(WalletModel *model)
{
    this->walletModel = model;
    UpdateTable();
}

void ProposalsDialog::UpdateTable()
{
    int columns = 12;
    int rows = 0;
    QStandardItemModel* model = new QStandardItemModel(rows, columns, this);
    model->setHorizontalHeaderItem(0, new QStandardItem(QString("Name")));
    model->setHorizontalHeaderItem(1, new QStandardItem(QString("Abstract")));
    model->setHorizontalHeaderItem(2, new QStandardItem(QString("Yes Votes")));
    model->setHorizontalHeaderItem(3, new QStandardItem(QString("No Votes")));
    model->setHorizontalHeaderItem(4, new QStandardItem(QString("Abstain")));
    model->setHorizontalHeaderItem(5, new QStandardItem(QString("Ratio")));
    model->setHorizontalHeaderItem(6, new QStandardItem(QString("TxID")));
    model->setHorizontalHeaderItem(7, new QStandardItem(QString("ID")));
    model->setHorizontalHeaderItem(8, new QStandardItem(QString("Location")));
    model->setHorizontalHeaderItem(9, new QStandardItem(QString("Start Block")));
    model->setHorizontalHeaderItem(10, new QStandardItem(QString("End Block")));
    model->setHorizontalHeaderItem(11, new QStandardItem(QString("Bits")));

    CVoteDB voteDB("r");
    int i = 0;
    for (auto it : mapProposals) {
        CVoteProposal proposal;
        if (voteDB.ReadProposal(it.first, proposal)) {
            //Filters
            bool hasStarted = proposal.GetStartHeight() < nBestHeight;
            bool isFinished = (proposal.GetStartHeight() + proposal.GetCheckSpan()) < nBestHeight;
            bool isActive = hasStarted && !isFinished;

            bool fUpcomingChecked = ui->checkBox_Upcoming->checkState() == Qt::CheckState::Checked;
            bool fActiveChecked = ui->checkBox_Active->checkState() == Qt::CheckState::Checked;
            bool fFinishedChecked = ui->checkBox_Finished->checkState() == Qt::CheckState::Checked;

            bool fInclude = false;
            if (fActiveChecked && isActive)
                fInclude = true;
            if (!hasStarted && fUpcomingChecked)
                fInclude = true;
            if (isFinished && fFinishedChecked)
                fInclude = true;
            if (!fInclude)
                continue;

            //Find the current status of the proposal's votes
            int nHeightEnd = proposal.GetStartHeight() + proposal.GetCheckSpan();
            int nHeightTally;
            if (nHeightEnd < nBestHeight)
                nHeightTally = nHeightEnd;
            else
                nHeightTally = nBestHeight;

            CVoteTally tally = FindBlockByHeight(nHeightTally)->tally;
            CVoteSummary summary;
            bool fTallySet = true;
            if (!tally.GetSummary(proposal.GetHash(), summary))
                fTallySet = false;
            QList<QStandardItem*> listItems;
            listItems.push_back(new QStandardItem(QString::fromStdString(proposal.GetName())));
            listItems.push_back(new QStandardItem(QString::fromStdString(proposal.GetDescription())));
            if (fTallySet) {
                int64_t nBlocksVoted = nHeightTally - summary.nBlockStart;
                double nAbstain = nBlocksVoted - summary.nNoTally - summary.nYesTally;
                double nRatio = 0;
                if (nBlocksVoted)
                    nRatio = (double)summary.nYesTally / ((double)nBlocksVoted - nAbstain);
               listItems.push_back(new QStandardItem(QString::number(summary.nYesTally)));
               listItems.push_back(new QStandardItem(QString::number(summary.nNoTally)));
               listItems.push_back(new QStandardItem(QString::number(nAbstain)));
               listItems.push_back(new QStandardItem(QString::number(nRatio)));
            } else {
                listItems.push_back(new QStandardItem(QString("n/a")));
                listItems.push_back(new QStandardItem(QString("n/a")));
                listItems.push_back(new QStandardItem(QString("n/a")));
                listItems.push_back(new QStandardItem(QString("n/a")));
            }
            listItems.push_back(new QStandardItem(QString::fromStdString(it.first.GetHex())));
            listItems.push_back(new QStandardItem(QString::fromStdString(proposal.GetHash().GetHex())));
            listItems.push_back(new QStandardItem(QString::number(proposal.GetShift())));
            listItems.push_back(new QStandardItem(QString::number(proposal.GetStartHeight())));
            listItems.push_back(new QStandardItem(QString::number(proposal.GetStartHeight() + proposal.GetCheckSpan())));
            listItems.push_back(new QStandardItem(QString::number(proposal.GetBitCount())));
            model->insertRow(i, listItems);
            i++;
        }
    }
    ui->tableView->setModel(model);
}

void ProposalsDialog::on_checkBox_Active_stateChanged(int arg1)
{
    UpdateTable();
}

void ProposalsDialog::on_checkBox_Upcoming_stateChanged(int arg1)
{
    UpdateTable();
}

void ProposalsDialog::on_checkBox_Finished_stateChanged(int arg1)
{
    UpdateTable();
}
