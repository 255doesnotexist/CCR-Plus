#include <QUrl>
#include <QDir>
#include <QFile>
#include <QMenu>
#include <QScrollBar>
#include <QHeaderView>
#include <QDomDocument>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QDesktopServices>

#include "common/global.h"
#include "common/player.h"
#include "common/problem.h"
#include "mainwindow/detailtable.h"

DetailTable::DetailTable(QWidget* parent) : QTableWidget(parent),
    is_scrollBar_at_bottom(false), is_locked(false), last_judge_timer(new QElapsedTimer()), rows(0)
{
    this->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
    this->setMinimumSize(QSize(320, 250));
    this->setFocusPolicy(Qt::NoFocus);
    this->setFrameShape(QFrame::NoFrame);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->setSelectionMode(QAbstractItemView::NoSelection);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->setStyleSheet(QLatin1String(
                            "QHeaderView"
                            "{"
                            "  background:#FFFFFF;"
                            "}"));

    this->horizontalHeader()->setDefaultSectionSize(45);
    this->horizontalHeader()->setMinimumSectionSize(45);
    this->horizontalHeader()->setMinimumHeight(25);
    this->horizontalHeader()->setStretchLastSection(true);
    this->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    this->verticalHeader()->setDefaultSectionSize(22);
    this->verticalHeader()->setMinimumSectionSize(22);
    this->verticalHeader()->setMinimumWidth(22);
    this->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    this->verticalHeader()->setDefaultAlignment(Qt::AlignRight | Qt::AlignVCenter);

    menu = new QMenu(this);
    action_in = new QAction("查看输入文件(&I)...", this);
    action_out = new QAction("查看输出文件(&O)...", this);
    action_sub = new QAction("查看提交文件(&S)...", this);

    connect(action_in,  &QAction::triggered, this, &DetailTable::onOpenInFile);
    connect(action_out, &QAction::triggered, this, &DetailTable::onOpenOutFile);
    connect(action_sub, &QAction::triggered, this, &DetailTable::onOpenSubmitFile);
    connect(this, &QWidget::customContextMenuRequested, this, &DetailTable::onContextMenuEvent);
}

DetailTable::~DetailTable()
{
    delete last_judge_timer;
}

void DetailTable::StartLastJudgeTimer()
{
     last_judge_timer->start();
}

void DetailTable::ClearDetail()
{
    this->clear();
    this->setRowCount(0);
    this->setColumnCount(2);
    this->setHorizontalHeaderLabels({"得分", "详情"});
    this->verticalScrollBar()->setValue(0);

    this->horizontalHeader()->setDefaultSectionSize(this->horizontalHeader()->sectionSizeHint(0));
    this->verticalHeader()->setDefaultSectionSize(0.9 * this->horizontalHeader()->height());

    is_scrollBar_at_bottom = false;
    is_show_detail = false;
    is_locked = false;
    rows = 0;

    player_at.clear();
    problem_at.clear();
    current_player = nullptr;
    current_problem = nullptr;
}



void DetailTable::adjustScrollBar()
{
    QCoreApplication::processEvents();
    QScrollBar* bar = this->verticalScrollBar();
    if (is_scrollBar_at_bottom) bar->setValue(bar->maximum());
}

void DetailTable::showProblemDetail(const Player* player, const Problem* problem)
{
    rows = this->rowCount();
    current_player = player;
    current_problem = problem;

    QString title = player->GetNameWithList();
    if (title == "std") title = QString("\"%1\" 的标程").arg(problem->Name()); else title += " - " + problem->Name();
    onAddTitleDetail(title);

    QFile file(Global::g_contest.result_path + problem->Name() + "/" + player->Name() + ".res");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        onAddNoteDetail("无测评结果", " ");
        return;
    }
    QDomDocument doc;
    if (!doc.setContent(&file))
    {
        file.close();
        onAddNoteDetail("无效的测评结果", " ");
        return;
    }
    QDomElement root = doc.documentElement();
    if (root.isNull() || root.tagName() != "task")
    {
        file.close();
        onAddNoteDetail("无效的测评结果", " ");
        return;
    }

    QDomNodeList list = root.childNodes();
    for (int i = 0; i < list.count(); i++)
        if (list.item(i).toElement().tagName() == "note")
            onAddNoteDetail(list.item(i).toElement().text(), root.attribute("state"));

    for (int i = 0, tot = 0, tasktot = 0; i < list.count(); i++)
    {
        QDomElement a = list.item(i).toElement();
        if (a.tagName() == "subtask")
        {
            QDomNodeList l = a.childNodes();
            int len = 0;
            for (int j = 0; j < l.count(); j++)
            {
                QDomElement b = l.item(j).toElement();
                if (b.tagName() == "point")
                {
                    onAddPointDetail(tot + 1, b.attribute("note"), b.attribute("state"), tot < problem->TestCaseCount() ?
                                                                                               problem->GetInOutString(problem->TestCaseAt(tot)) :
                                                                                               "", ++len);
                    tot++;
                }
            }

            if (len) onAddScoreDetail(len, a.attribute("score").toInt(), tasktot < problem->SubtaskCount() ?
                                                                                   problem->SubtaskAt(tasktot)->Score() :
                                                                                   0);
            tasktot++;
        }
    }
    file.close();
}



void DetailTable::onAddTitleDetail(const QString& title)
{
    if (Global::g_is_contest_closed) return;

    is_scrollBar_at_bottom = this->verticalScrollBar()->value() >= this->verticalScrollBar()->maximum() - 5;

    QTableWidgetItem* tmp = new QTableWidgetItem(title);
    tmp->setForeground(Qt::white);
    tmp->setBackground(QColor(120, 120, 120));
    tmp->setToolTip(title);

    this->insertRow(rows);
    this->setItem(rows, 0, tmp);
    this->setSpan(rows, 0, 1, 2);
    this->setVerticalHeaderItem(rows, new QTableWidgetItem);
    player_at.append(current_player);
    problem_at.append(current_problem);
    rows++;

    if (!is_show_detail) this->adjustScrollBar();
}

void DetailTable::onAddNoteDetail(const QString& note, const QString& state)
{
    if (Global::g_is_contest_closed) return;

    is_scrollBar_at_bottom = this->verticalScrollBar()->value() >= this->verticalScrollBar()->maximum() - 5;

    QTableWidgetItem* tmp = new QTableWidgetItem(note);
    tmp->setToolTip(tmp->text());
    tmp->setForeground(QColor(80, 80, 80));
    tmp->setBackground(QColor(180, 180, 180));
    if (state == "E") {
        tmp->setForeground(QColor(0, 0, 0));
        tmp->setBackground(QColor(227, 58, 218));
    }
    if (state == " " || state.isEmpty()) {
        tmp->setForeground(QColor(100, 100, 100));
        tmp->setBackground(QColor(235, 235, 235));
    }
    if (state.isEmpty()) {
        tmp->setTextAlignment(Qt::AlignCenter);
    }

    int a = tmp->text().split('\n').count(), b = a == 1 ? this->verticalHeader()->defaultSectionSize() : std::min(a, 4) * QFontMetrics(this->font()).height() + 6;
    this->insertRow(rows);
    this->setItem(rows, 0, tmp);
    this->setSpan(rows, 0, 1, 2);
    this->setVerticalHeaderItem(rows, new QTableWidgetItem);
    this->verticalHeader()->resizeSection(rows, b);
    player_at.append(current_player);
    problem_at.append(current_problem);
    rows++;

    if (!is_show_detail) this->adjustScrollBar();
}

void DetailTable::onAddPointDetail(int num, const QString& note, const QString& state, const QString& inOut, int subTaskLen)
{
    if (Global::g_is_contest_closed) return;

    is_scrollBar_at_bottom = this->verticalScrollBar()->value() >= this->verticalScrollBar()->maximum() - 5;

    QTableWidgetItem* tmp = new QTableWidgetItem(note);
    tmp->setToolTip(tmp->text());

    QColor o(255, 255, 255);
    if (state == "conf") o.setRgb(0, 161, 241); // Configuration
    if (state.length() == 1)
        switch (state[0].toLatin1())
        {
        case 'A':
            o.setRgb(51, 185, 6); // AC
            break;
        case 'C':
        case 'E':
            o.setRgb(227, 58, 218); // Error
            break;
        case 'I':
        case 'U':
            o.setRgb(235, 235, 235); // Ignore/UnSubmit
            break;
        case 'M':
        case 'R':
            o.setRgb(247, 63, 63); // MLE/RE
            break;
        case 'O':
            o.setRgb(180, 180, 180); // No Output
            break;
        case 'P':
            o.setRgb(143, 227, 60); // Partial
            break;
        case 'W':
            o.setRgb(246, 123, 20); // WA
            break;
        case 'T':
            o.setRgb(255, 187, 0); // TLE
            break;
        }
    tmp->setBackground(o);

    this->insertRow(rows);
    this->setItem(rows, 1, tmp);

    QTableWidgetItem* t = new QTableWidgetItem(QString::number(num));
    t->setToolTip(inOut);
    this->setVerticalHeaderItem(rows, t);

    if (subTaskLen > 1) this->setSpan(rows - subTaskLen + 1, 0, subTaskLen, 1);
    player_at.append(current_player);
    problem_at.append(current_problem);
    rows++;

    if (!is_show_detail) this->adjustScrollBar();
}

void DetailTable::onAddScoreDetail(int subTaskLen, int score, int sumScore)
{
    if (Global::g_is_contest_closed) return;

    QTableWidgetItem* tmp = new QTableWidgetItem(QString::number(score));
    tmp->setTextAlignment(Qt::AlignCenter);
    tmp->setToolTip(tmp->text());
    tmp->setBackground(Global::GetRatioColor(235, 235, 235, 0, 161, 241, score, sumScore));
    this->setItem(rows - subTaskLen, 0, tmp);
}

void DetailTable::onShowDetail(int row, int column)
{
    if (is_locked || (last_judge_timer->isValid() && last_judge_timer->elapsed() < 1000)) return;
    ClearDetail();
    is_show_detail = true;

    row = Global::GetLogicalRow(row);
    if (column > 1)
        showProblemDetail(Global::g_contest.players[row], Global::g_contest.problems[column - 2]);
    else
    {
        for (auto i : Global::g_contest.problem_order)
            showProblemDetail(Global::g_contest.players[row], Global::g_contest.problems[i]);
    }

    is_show_detail = false;
}

void DetailTable::onShowConfigurationDetail()
{
    is_show_detail = true;
    rows = this->rowCount();
    for (auto i : Global::g_contest.problem_order)
    {
        const Problem* prob = current_problem = Global::g_contest.problems[i];
        onAddTitleDetail(QString("\"%1\" 的配置结果").arg(prob->Name()));

        int t = 0;
        for (int i = 0; i < prob->SubtaskCount(); i++)
        {
            const Subtask* sub = prob->SubtaskAt(i);
            int len = 0;
            for (auto point : *sub) onAddPointDetail(++t, prob->GetInOutString(point), "conf", prob->GetInOutString(point), ++len);
            onAddScoreDetail(sub->Size(), sub->Score(), sub->Score());
        }
    }

    is_show_detail = false;
}



static QString inFileByAction, outFileByAction, submitFileByAction;

void DetailTable::onOpenInFile()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(inFileByAction));
}

void DetailTable::onOpenOutFile()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(outFileByAction));
}

void DetailTable::onOpenSubmitFile()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(submitFileByAction));
}

void DetailTable::onContextMenuEvent(const QPoint& pos)
{
    action_in->setEnabled(false);
    action_out->setEnabled(false);
    action_sub->setEnabled(false);
    menu->clear();

    QTableWidgetItem* item = this->itemAt(pos);
    if (!item) return;

    int row = item->row();
    const Player* player = player_at[row];
    const Problem* problem = problem_at[row];
    if (!problem) return;

    if (item->foreground().color() == Qt::white)  // title item
    {
        if (!player)
            inFileByAction = Global::g_contest.data_path + problem->Name() + "/";
        else
            inFileByAction = Global::g_contest.src_path + player->Name() + "/" + problem->Name() + "/";

        action_in->setText("打开目录(&O)...");
        if (QDir(inFileByAction).exists()) action_in->setEnabled(true);

        menu->addAction(action_in);
        menu->popup(QCursor::pos());
    }
    else if (item->column() == 1) // test point item
    {
        int id = this->verticalHeaderItem(row)->text().toInt() - 1;
        if (id < 0 || id >= problem->TestCaseCount()) return;

        QString in = problem->TestCaseAt(id)->InFile();
        QString out = problem->TestCaseAt(id)->OutFile();
        QString sub = problem->TestCaseAt(id)->SubmitFile();
        inFileByAction = Global::g_contest.data_path + problem->Name() + "/" + in;
        outFileByAction = Global::g_contest.data_path + problem->Name() + "/" + out;
        if (player) submitFileByAction = Global::g_contest.src_path + player->Name() + "/" + problem->Name() + "/" + sub;

        action_in->setText(QString("查看输入文件 \"%1\" (&I)...").arg(in));
        if (QFile(inFileByAction).exists()) action_in->setEnabled(true);

        action_out->setText(QString("查看输出文件 \"%1\" (&O)...").arg(out));
        if (QFile(outFileByAction).exists()) action_out->setEnabled(true);

        menu->addAction(action_in);
        menu->addAction(action_out);

        if (problem->Type() == Global::AnswersOnly && player)
        {
            action_sub->setText(QString("查看提交文件 \"%1\" (&S)...").arg(sub));
            if (QFile(submitFileByAction).exists()) action_sub->setEnabled(true);

            menu->addAction(action_sub);
        }

        menu->popup(QCursor::pos());
    }
}
