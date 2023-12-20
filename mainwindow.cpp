#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFontDatabase>
#include <QFont>
#include <QCloseEvent>
#include "childwnd.h"
#include <QMdiSubWindow>
#include <QFileDialog>
#include <QColorDialog>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initMainWindow();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initMainWindow()
{
    //初始化字号列表项
    QFontDatabase fontdb;
    foreach (int fontsize, fontdb.standardSizes()) {
        ui->sizeComboBox->addItem(QString::number(fontsize));
    }

    QFont defFont;     //当前应用程序默认的字体
    QString sFontSize; //当前应用程序默认字体的字号大小
    int defFontSize;   //当前应用程序默认字体的字号
    int defFontindex;  //当前字号在组合框中的索引号

    defFont = QApplication::font();     //获取默认的字号
    defFontSize = defFont.pointSize();  //获取默认字号的整型变量
    sFontSize = QString::number(defFontSize);
    defFontindex = ui->sizeComboBox->findText(sFontSize);

    ui->sizeComboBox->setCurrentIndex(defFontindex);//显示默认字号

    //设置多文档区域滚动条
    ui->mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    refreshMenus();
    connect(ui->mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::refreshMenus);

    addSubWndListMenu();
    connect(ui->menu_W, &QMenu::aboutToShow, this, &MainWindow::addSubWndListMenu);

    //创建信号映射器
    m_WndMapper = new QSignalMapper(this);
    connect(m_WndMapper, SIGNAL(mapped(QWidget*)), this, SLOT(setActiveSubWindow(QWidget*)));//map信号传来触发槽函数

    //实现段落对齐，创建一个组，保证互斥性，一次只能选择一种
    QActionGroup* alignGroup = new QActionGroup(this);
    alignGroup->addAction(ui->leftAlignAction);
    alignGroup->addAction(ui->rightAlignAction);
    alignGroup->addAction(ui->centerAction);
    alignGroup->addAction(ui->justifyAction);

}

void MainWindow::docNew()
{
    ChildWnd *childwnd = new ChildWnd();
    ui->mdiArea->addSubWindow(childwnd);
    connect(childwnd, SIGNAL(copyAvailable(bool)), ui->cutAction, SLOT(setEnabled(bool)));
    connect(childwnd, SIGNAL(copyAvailable(bool)), ui->copyAction, SLOT(setEnabled(bool)));

    childwnd->newDoc();
    childwnd->show();
    formatEnabled();
}

void MainWindow::docOpen()
{
    QString docName = QFileDialog::getOpenFileName(this, "打开文档", "", "文本文件(*.txt);;" "HTML文件(*.html *.htm;;" "所有文件(*.*)");

    if (!docName.isEmpty()) {
        QMdiSubWindow* existWnd = findChildWnd(docName);
        if (existWnd) {
            ui->mdiArea->setActiveSubWindow(existWnd);
            return;
        }
        ChildWnd* childWnd = new ChildWnd;
        ui->mdiArea->addSubWindow(childWnd);

        connect(childWnd, SIGNAL(copyAvailable(bool)), ui->cutAction, SLOT(setEnabled(bool)));
        connect(childWnd, SIGNAL(copyAvailable(bool)), ui->copyAction, SLOT(setEnabled(bool)));

        if (childWnd->loadDoc(docName)) {
            statusBar()->showMessage("文档已打开", 3000);
            childWnd->show();
            formatEnabled();
        } else {
            childWnd->close();
        }
    }
}

void MainWindow::docSave()
{
    if (activateChildWnd() && activateChildWnd()->saveDoc())
        statusBar() ->showMessage("保存成功", 3000);
}

void MainWindow::docSaveAs()
{
    if (activateChildWnd() && activateChildWnd()->saveAsDoc())
        statusBar()->showMessage("保存成功", 3000);
}

void MainWindow::docPrint()
{
    QPrinter pter(QPrinter::HighResolution);
    QPrintDialog *ddlg = new QPrintDialog(&pter, this);

    if (activateChildWnd())
        ddlg->setOption(QAbstractPrintDialog::PrintSelection, true);
    ddlg->setWindowTitle("打印文档");

    ChildWnd* childWnd = activateChildWnd();
    if (ddlg->exec() == QDialog::Accepted)
        childWnd->print(&pter);

    delete ddlg;
}

void MainWindow::docPrintPreview()
{
    QPrinter pter;
    QPrintPreviewDialog preview(&pter, this);
    connect(&preview, SIGNAL(paintRequested(QPrinter*)), this, SLOT(printPreview(QPrinter*)));

    preview.exec();
}

void MainWindow::docUndo()
{
    if (activateChildWnd()) activateChildWnd()->undo();
}

void MainWindow::docRedo()
{
    if (activateChildWnd()) activateChildWnd()->redo();
}

void MainWindow::docCut()
{
    if (activateChildWnd()) activateChildWnd()->cut();
}

void MainWindow::docCopy()
{
    if (activateChildWnd()) activateChildWnd()->copy();
}

void MainWindow::docPaste()
{
    if (activateChildWnd()) activateChildWnd()->paste();
}

void MainWindow::textBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(ui->boldAction->isChecked() ? QFont::Bold : QFont::Normal);
    if (activateChildWnd())
        activateChildWnd()->setFormatOnSelectedWord(fmt);
}

void MainWindow::textItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(ui->italicAction->isChecked());
    if (activateChildWnd())
        activateChildWnd()->setFormatOnSelectedWord(fmt);
}

void MainWindow::textUnderLine()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(ui->underlineAction->isChecked());
    if (activateChildWnd())
        activateChildWnd()->setFormatOnSelectedWord(fmt);
}

void MainWindow::textFamily(const QString &fmly)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(fmly);
    if (activateChildWnd())
        activateChildWnd()->setFormatOnSelectedWord(fmt);
}

void MainWindow::textSize(const QString &ps)
{
    qreal pointSize = ps.toFloat();
    if (ps.toFloat() > 0)
    {
        QTextCharFormat fmt;
        fmt.setFontPointSize(pointSize);
        if (activateChildWnd())
            activateChildWnd()->setFormatOnSelectedWord(fmt);
    }
}

void MainWindow::textColor()
{
    if (activateChildWnd()) {
        QColor color = QColorDialog::getColor(activateChildWnd()->textColor(), this);
        if (!color.isValid()) return;

        QTextCharFormat fmt;
        fmt.setForeground(color);
        activateChildWnd()->setFormatOnSelectedWord(fmt);

        QPixmap pix(16, 16);
        pix.fill(color);
        ui->colorAction->setIcon(pix);
    }
}

int MainWindow::paraStyle(int nStyle)
{
    if (activateChildWnd())
        activateChildWnd()->setParaStyle(nStyle);
}

void MainWindow::formatEnabled()
{
    ui->boldAction->setEnabled(true);
    ui->italicAction->setEnabled(true);
    ui->underlineAction->setEnabled(true);
    ui->leftAlignAction->setEnabled(true);
    ui->centerAction->setEnabled(true);
    ui->rightAlignAction->setEnabled(true);
    ui->justifyAction->setEnabled(true);
    ui->colorAction->setEnabled(true);
}

ChildWnd *MainWindow::activateChildWnd()
{
    //获取当前活跃的子窗口
    QMdiSubWindow* actWnd = ui->mdiArea->activeSubWindow();
    if (actWnd)
        return qobject_cast<ChildWnd*>(actWnd->widget());//以部件的形式返回
    return 0;
}

QMdiSubWindow *MainWindow::findChildWnd(const QString &docName)
{
    QString strFile = QFileInfo(docName).canonicalFilePath();
    foreach (QMdiSubWindow* subWnd, ui->mdiArea->subWindowList()) {
        ChildWnd* chilwnd = qobject_cast<ChildWnd*>(subWnd->widget());
        if (chilwnd->m_CurDocPath == strFile) return subWnd;
    }
    return 0;
}

void MainWindow::on_newAction_triggered()
{
    docNew();
}

void MainWindow::refreshMenus()
{
    bool hasChild;
    hasChild = (activateChildWnd() != 0);

    ui->saveAction->setEnabled(hasChild);
    ui->savaAsAction->setEnabled(hasChild);
    ui->printAction->setEnabled(hasChild);
    ui->printPreviewAction->setEnabled(hasChild);
    ui->pasteAction->setEnabled(hasChild);
    ui->closeAction->setEnabled(hasChild);
    ui->closeAllAction->setEnabled(hasChild);
    ui->titleAction->setEnabled(hasChild);
    ui->nextAction->setEnabled(hasChild);
    ui->previousAction->setEnabled(hasChild);

    //文档打开且有内容选中
    bool hasSelect = ( activateChildWnd() && activateChildWnd()->textCursor().hasSelection());
    ui->cutAction->setEnabled(hasSelect);
    ui->copyAction->setEnabled(hasSelect);
    ui->boldAction->setEnabled(hasSelect);
    ui->italicAction->setEnabled(hasSelect);
    ui->underlineAction->setEnabled(hasSelect);
    ui->leftAlignAction->setEnabled(hasSelect);
    ui->centerAction->setEnabled(hasSelect);
    ui->rightAlignAction->setEnabled(hasSelect);
    ui->justifyAction->setEnabled(hasSelect);
    ui->colorAction->setEnabled(hasSelect);




}

void MainWindow::addSubWndListMenu()
{
    //清空之前的信息并把需要的加上
    ui->menu_W->clear();
    ui->menu_W->addAction(ui->closeAction);
    ui->menu_W->addAction(ui->closeAllAction);
    ui->menu_W->addSeparator();
    ui->menu_W->addAction(ui->titleAction);
    ui->menu_W->addAction(ui->cascadeAction);
    ui->menu_W->addSeparator();
    ui->menu_W->addAction(ui->nextAction);
    ui->menu_W->addAction(ui->previousAction);

    QList<QMdiSubWindow*> wnds = ui->mdiArea->subWindowList();
    if (!wnds.isEmpty()) ui->menu_W->addSeparator();

    for (int i = 0; i < wnds.size(); ++ i) {
       ChildWnd* childWnd = qobject_cast<ChildWnd*>(wnds.at(i)->widget());
       QString menuitem_text;
       menuitem_text = QString("%1 %2").arg(i + 1).arg(childWnd->getCurDocName());

       QAction *menuitem_act = ui->menu_W->addAction(menuitem_text);
       menuitem_act->setCheckable(true);//将菜单项设置为可以被选中
       menuitem_act->setChecked(childWnd == activateChildWnd());//根据当前的窗口设置激活的状态

       connect(menuitem_act, SIGNAL(triggered(bool)), m_WndMapper, SLOT(map()));//点击菜单发出信号，执行map方法，map方法触发上面的map信号
       m_WndMapper->setMapping(menuitem_act, wnds.at(i));
    }
    formatEnabled();
}

void MainWindow::on_closeAction_triggered()
{
    ui->mdiArea->closeActiveSubWindow();
}

void MainWindow::on_closeAllAction_triggered()
{
    ui->mdiArea->closeAllSubWindows();
}

void MainWindow::on_titleAction_triggered()
{
    ui->mdiArea->tileSubWindows();
}

void MainWindow::on_cascadeAction_triggered()
{
    ui->mdiArea->cascadeSubWindows();
}

void MainWindow::on_nextAction_triggered()
{
    ui->mdiArea->activateNextSubWindow();
}

void MainWindow::on_previousAction_triggered()
{
    ui->mdiArea->activatePreviousSubWindow();
}

void MainWindow::setActiveSubWindow(QWidget *wnd)
{
    if (!wnd) return;
    ui->mdiArea->setActiveSubWindow(qobject_cast<QMdiSubWindow*>(wnd));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    ui->mdiArea->closeAllSubWindows();
    if (ui->mdiArea->currentSubWindow())
        event->ignore();        //忽略此事件
    else
        event->accept();        //接受此事件
}

void MainWindow::on_openAction_triggered()
{
    docOpen();
}

void MainWindow::on_saveAction_triggered()
{
    docSave();
}

void MainWindow::on_savaAsAction_triggered()
{
    docSaveAs();
}

void MainWindow::on_undoAction_triggered()
{
    docUndo();
}

void MainWindow::on_redoAction_triggered()
{
    docRedo();
}

void MainWindow::on_cutAction_triggered()
{
    docCut();
}

void MainWindow::on_copyAction_triggered()
{
    docCopy();
}

void MainWindow::on_pasteAction_triggered()
{
    docPaste();
}

void MainWindow::on_boldAction_triggered()
{
    textBold();
}

void MainWindow::on_italicAction_triggered()
{
    textItalic();
}

void MainWindow::on_underlineAction_triggered()
{
    textUnderLine();
}

void MainWindow::on_fontComboBox_activated(const QString &arg1)
{
    textFamily(arg1);
}

void MainWindow::on_sizeComboBox_activated(const QString &arg1)
{
    textSize(arg1);
}

void MainWindow::on_leftAlignAction_triggered()
{
    if (activateChildWnd())
        activateChildWnd()->setAlignOfDocumentText(1);
}

void MainWindow::on_rightAlignAction_triggered()
{
    if (activateChildWnd())
        activateChildWnd()->setAlignOfDocumentText(2);
}

void MainWindow::on_centerAction_triggered()
{
    if (activateChildWnd())
        activateChildWnd()->setAlignOfDocumentText(3);
}

void MainWindow::on_justifyAction_triggered()
{
    if (activateChildWnd())
        activateChildWnd()->setAlignOfDocumentText(4);
}

void MainWindow::on_colorAction_triggered()
{
    textColor();
}

void MainWindow::on_comboBox_activated(int index)
{
    paraStyle(index);
}

void MainWindow::on_printAction_triggered()
{
    docPrint();
}

void MainWindow::printPreview(QPrinter *printer)
{
    activateChildWnd()->print(printer);
}

void MainWindow::on_printPreviewAction_triggered()
{
    docPrintPreview();
}
