/*------- 个人信息管理模块 --------*/

#ifndef USERINFOWIDGET_H
#define USERINFOWIDGET_H

#include <QWidget>
#include <QtSql>
#include <QTableView>
#include <QTimer>
#include <QSqlDatabase>

#include "helper/learningpatterntest.h"

namespace Ui {
class UserInfoWidget;
}

class UserInfoWidget : public QWidget
{
    Q_OBJECT

public:
    UserInfoWidget(QWidget *parent = 0);
    ~UserInfoWidget();

protected:
    //mouse func
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

private slots:
    void updateTimeSlot();
    void modifyInformationSlot();
    void saveInformationSlot();
    void on_learningTestButton_clicked();

private:
    void openDatabase();
    void initUI();
    void init();

private:
    Ui::UserInfoWidget* ui;
    QSqlDatabase db;
    bool havePathTab;
    bool haveBehaviorTab;
    QWidget *pathWidget;
    QWidget *behaviorWidget;
    QTableView *pathTableView;
    QTableView *behaviorTableView;
    LearningPatternTest* testWidget = nullptr;
    QTimer *timer = nullptr;
    //mouse moving
    bool        mMove;
    QPoint      mPos;
};

#endif // userinfowidget_H
