#ifndef CONFIGURETABLE_H
#define CONFIGURETABLE_H

#include "global.h"

#include <QTableView>
#include <QStandardItemModel>
#include <QStyledItemDelegate>

class ConfigureTableItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ConfigureTableItemDelegate(const QStringList& list, QObject* parent = 0) :
        QStyledItemDelegate(parent), problem_list(list) {}
    ~ConfigureTableItemDelegate() {}

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

protected:
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

private:
    QStringList problem_list;

    static QRect checkBoxRect(const QStyleOptionViewItem& viewItemStyleOptions);
};



class ConfigureTable : public QTableView
{
    Q_OBJECT
public:
    explicit ConfigureTable(const QStringList& list, QWidget* parent = 0);
    ~ConfigureTable() {}

    // Getter member functions
    QString ItemText(int r, int c) const { return model->item(r, c)->text(); }
    QVariant ItemData(int r, int c) const { return model->item(r, c)->data(Qt::DisplayRole); }
    bool IsItemChanged(int r, int c) const { return model->item(r, c)->font().bold(); }
    bool IsColumnChanged(int c) const { return model->horizontalHeaderItem(c)->font().bold(); }

    // Setter member functions
    void SetItemText(int r, int c, const QString& text) { SetItemData(r, c, text); }
    void SetItemData(int r, int c, const QVariant& value)
    {
        model->setData(model->index(r, c), value, Qt::DisplayRole);
        model->setData(model->index(r, c), Qt::AlignCenter, Qt::TextAlignmentRole);
        if (r == 4)
            model->setData(model->index(r, c), value.toBool() ? tr("true") : tr("false"), Qt::ToolTipRole);
        else
            model->setData(model->index(r, c), value.toString(), Qt::ToolTipRole);
    }
    void SetItemBold(int r, int c) { model->setData(model->index(r, c), Global::BOLD_FONT, Qt::FontRole); }
    void SetItemChanged(int r, int c)
    {
        SetItemBold(r, c);
        model->setHeaderData(c, Qt::Horizontal, Global::BOLD_FONT, Qt::FontRole);
    }

private:
    QStandardItemModel* model;
    QStringList problem_list;
    bool is_changing_data;

    void setModelDataNew(int column);
    void setModelData(int column);
    void loadProblems();

private slots:
    void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
};

#endif // CONFIGURETABLE_H