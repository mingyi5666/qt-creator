/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "watchwindow.h"
#include "watchhandler.h"

#include "debuggeractions.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtGui/QAction>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QItemDelegate>
#include <QtGui/QLineEdit>
#include <QtGui/QMenu>
#include <QtGui/QResizeEvent>

using namespace Debugger::Internal;


/////////////////////////////////////////////////////////////////////
//
// WatchDelegate
//
/////////////////////////////////////////////////////////////////////

class WatchDelegate : public QItemDelegate
{
public:
    WatchDelegate(QObject *parent) : QItemDelegate(parent) {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
        const QModelIndex &) const
    {
        return new QLineEdit(parent);
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const
    {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
        QTC_ASSERT(lineEdit, return);
        if (index.column() == 1) 
            lineEdit->setText(index.model()->data(index, Qt::DisplayRole).toString());
        else
            lineEdit->setText(index.model()->data(index, ExpressionRole).toString());
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
        const QModelIndex &index) const
    {
        //qDebug() << "SET MODEL DATA";
        QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor);
        QTC_ASSERT(lineEdit, return);
        QString value = lineEdit->text();
        QString exp = model->data(index, ExpressionRole).toString();
        model->setData(index, value, Qt::EditRole);
        if (index.column() == 1) {
            // the value column
            theDebuggerAction(AssignValue)->trigger(QString(exp + '=' + value));
        } else if (index.column() == 2) {
            // the type column
            theDebuggerAction(AssignType)->trigger(QString(exp + '=' + value));
        } else if (index.column() == 0) {
            // the watcher name column
            theDebuggerAction(RemoveWatchExpression)->trigger(exp);
            theDebuggerAction(WatchExpression)->trigger(value);
        }
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
        const QModelIndex &) const
    {
        editor->setGeometry(option.rect);
    }
};


/////////////////////////////////////////////////////////////////////
//
// WatchWindow
//
/////////////////////////////////////////////////////////////////////

WatchWindow::WatchWindow(Type type, QWidget *parent)
    : QTreeView(parent), m_alwaysResizeColumnsToContents(true), m_type(type)
{
    m_grabbing = false;

    QAction *act = theDebuggerAction(UseAlternatingRowColors);
    setWindowTitle(tr("Locals and Watchers"));
    setAlternatingRowColors(act->isChecked());
    setIndentation(indentation() * 9/10);
    setUniformRowHeights(true);
    setItemDelegate(new WatchDelegate(this));
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);

    connect(act, SIGNAL(toggled(bool)),
        this, SLOT(setAlternatingRowColorsHelper(bool)));
    connect(this, SIGNAL(expanded(QModelIndex)), 
        this, SLOT(expandNode(QModelIndex))); 
    connect(this, SIGNAL(collapsed(QModelIndex)), 
        this, SLOT(collapseNode(QModelIndex))); 
} 
 
void WatchWindow::expandNode(const QModelIndex &idx) 
{ 
    model()->setData(idx, true, ExpandedRole);
} 
 
void WatchWindow::collapseNode(const QModelIndex &idx) 
{ 
    model()->setData(idx, false, ExpandedRole);
}

void WatchWindow::reset()
{
    QTreeView::reset(); 
    setRootIndex(model()->index(0, 0, QModelIndex()));
}

void WatchWindow::keyPressEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key_Delete && m_type == WatchersType) {
        QModelIndex idx = currentIndex();
        QModelIndex idx1 = idx.sibling(idx.row(), 0);
        QString exp = model()->data(idx1).toString();
        theDebuggerAction(RemoveWatchExpression)->trigger(exp);
    } else if (ev->key() == Qt::Key_Return
            && ev->modifiers() == Qt::ControlModifier
            && m_type == LocalsType) {
        QModelIndex idx = currentIndex();
        QModelIndex idx1 = idx.sibling(idx.row(), 0);
        QString exp = model()->data(idx1).toString();
        theDebuggerAction(WatchExpression)->trigger(exp);
    }
    QTreeView::keyPressEvent(ev);
}

void WatchWindow::dragEnterEvent(QDragEnterEvent *ev)
{
    //QTreeView::dragEnterEvent(ev);
    if (ev->mimeData()->hasFormat("text/plain")) {
        ev->setDropAction(Qt::CopyAction);
        ev->accept();
    }
}

void WatchWindow::dragMoveEvent(QDragMoveEvent *ev)
{
    //QTreeView::dragMoveEvent(ev);
    if (ev->mimeData()->hasFormat("text/plain")) {
        ev->setDropAction(Qt::CopyAction);
        ev->accept();
    }
}

void WatchWindow::dropEvent(QDropEvent *ev)
{
    if (ev->mimeData()->hasFormat("text/plain")) {
        theDebuggerAction(WatchExpression)->trigger(ev->mimeData()->text());
        //ev->acceptProposedAction();
        ev->setDropAction(Qt::CopyAction);
        ev->accept();
    }
    //QTreeView::dropEvent(ev);
}

void WatchWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QModelIndex idx = indexAt(ev->pos());
    QModelIndex mi0 = idx.sibling(idx.row(), 0);
    QModelIndex mi1 = idx.sibling(idx.row(), 1);
    QModelIndex mi2 = idx.sibling(idx.row(), 2);
    QString exp = model()->data(mi0).toString();
    QString type = model()->data(mi2).toString();

    QStringList alternativeFormats = 
        model()->data(mi0, TypeFormatListRole).toStringList();
    int typeFormat = 
        model()->data(mi0, TypeFormatRole).toInt();
    int individualFormat = 
        model()->data(mi0, IndividualFormatRole).toInt();

    QMenu typeFormatMenu(tr("Change format for type '%1'").arg(type));
    QMenu individualFormatMenu(tr("Change format for expression '%1'").arg(exp));
    QList<QAction *> typeFormatActions;
    QList<QAction *> individualFormatActions;
    for (int i = 0; i != alternativeFormats.size(); ++i) {
        const QString format = alternativeFormats.at(i);
        QAction *act = new QAction(format, &typeFormatMenu);
        act->setCheckable(true);
        if (i == typeFormat)
            act->setChecked(true);
        typeFormatMenu.addAction(act);
        typeFormatActions.append(act);
        act = new QAction(format, &individualFormatMenu);
        act->setCheckable(true);
        if (i == individualFormat)
            act->setChecked(true);
        individualFormatMenu.addAction(act);
        individualFormatActions.append(act);
    }
    //typeFormatMenu.setActive(!alternativeFormats.isEmpty());
    //individualFormatMenu.setActive(!alternativeFormats.isEmpty());

    QMenu menu;
    QAction *act1 = new QAction(tr("Adjust column widths to contents"), &menu);
    QAction *act2 = new QAction(tr("Always adjust column widths to contents"), &menu);

    act2->setCheckable(true);
    act2->setChecked(m_alwaysResizeColumnsToContents);

    //QAction *act4 = theDebuggerAction(WatchExpressionInWindow);
    //menu.addAction(act4);

    QAction *act3 = new QAction(tr("Insert new watch item"), &menu); 
    QAction *act4 = new QAction(tr("Select widget to watch"), &menu);

    menu.addAction(act1);
    menu.addAction(act2);
    menu.addSeparator();
    int atype = (m_type == LocalsType) ? WatchExpression : RemoveWatchExpression;
    menu.addAction(theDebuggerAction(atype)->updatedAction(exp));

    menu.addAction(act3);
    menu.addAction(act4);
    menu.addMenu(&typeFormatMenu);
    menu.addMenu(&individualFormatMenu);

    menu.addSeparator();
    menu.addAction(theDebuggerAction(RecheckDebuggingHelpers));
    menu.addAction(theDebuggerAction(UseDebuggingHelpers));
    menu.addSeparator();
    menu.addAction(theDebuggerAction(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());

    if (act == act1) {
        resizeColumnsToContents();
    } else if (act == act2) {
        setAlwaysResizeColumnsToContents(!m_alwaysResizeColumnsToContents);
    } else if (act == act3) {
        theDebuggerAction(WatchExpression)
            ->trigger(WatchHandler::watcherEditPlaceHolder());
    } else if (act == act4) {
        grabMouse(Qt::CrossCursor);
        m_grabbing = true;
    } else { 
        for (int i = 0; i != alternativeFormats.size(); ++i) {
            if (act == typeFormatActions.at(i))
                model()->setData(mi1, i, TypeFormatRole);
            else if (act == individualFormatActions.at(i))
                model()->setData(mi1, i, IndividualFormatRole);
        }
    }
}

void WatchWindow::resizeColumnsToContents()
{
    resizeColumnToContents(0);
    resizeColumnToContents(1);
}

void WatchWindow::setAlwaysResizeColumnsToContents(bool on)
{
    if (!header())
        return;
    m_alwaysResizeColumnsToContents = on;
    QHeaderView::ResizeMode mode = on 
        ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    header()->setResizeMode(0, mode);
    header()->setResizeMode(1, mode);
}

bool WatchWindow::event(QEvent *ev)
{
    if (m_grabbing && ev->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mev = static_cast<QMouseEvent *>(ev);
        m_grabbing = false;
        releaseMouse();
        theDebuggerAction(WatchPoint)->trigger(mapToGlobal(mev->pos()));
    }
    return QTreeView::event(ev);
}

void WatchWindow::editItem(const QModelIndex &idx)
{
    Q_UNUSED(idx); // FIXME
}

void WatchWindow::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);

    setRootIsDecorated(true);
    header()->setDefaultAlignment(Qt::AlignLeft);
    header()->setResizeMode(QHeaderView::ResizeToContents);
    if (m_type != LocalsType)
        header()->hide();

    connect(model, SIGNAL(layoutChanged()), this, SLOT(resetHelper()));
}

void WatchWindow::resetHelper()
{
    resetHelper(model()->index(0, 0));
}

void WatchWindow::resetHelper(const QModelIndex &idx)
{
    if (model()->data(idx, ExpandedRole).toBool()) {
        //qDebug() << "EXPANDING " << model()->data(idx, INameRole);
        expand(idx);
        for (int i = 0, n = model()->rowCount(idx); i != n; ++i) {
            QModelIndex idx1 = model()->index(i, 0, idx);
            resetHelper(idx1);
        }
    } else {
        //qDebug() << "COLLAPSING " << model()->data(idx, INameRole);
        collapse(idx);
    }
}
