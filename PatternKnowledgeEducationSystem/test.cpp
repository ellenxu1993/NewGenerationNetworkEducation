#include "stable.h"
#include <QTimer>
#include <QDateTime>
#include <QRadioButton>
#include <QMessageBox>

#include "test.h"
#include "ui_test.h"
#include "helper/user.h"
#include "helper/myheaders.h"

extern User myUser;
extern QString currentKid;
extern QString currentCid;

Test::Test(QWidget *parent)
    : QWidget(parent), ui(new Ui::Test), timer1(new QTimer(this)), timer2(new QTimer(this))
{
    ui->setupUi(this);
    initUI();
    init();

    connect(ui->buttonClose, &QPushButton::clicked, this, &Test::close);                                 // 点击关闭
    connect(ui->buttonMin, &QPushButton::clicked, this, &Test::showMinimized);                           // 点击最小化
    connect(timer1, &QTimer::timeout, this, &Test::timeUpdateSlot);//更新系统时间
    connect(ui->submitButton, &QPushButton::clicked, this, &Test::submitTestSlot);      //提交测试
    connect(startButton, &QPushButton::clicked, this, &Test::startTestSlot);            //点击开始测试按钮进入测试
}

Test::~Test()
{
    delete ui;
    QString connectName = db.connectionName();
    db = QSqlDatabase();
    db.removeDatabase(connectName);
    db.close();
}

void Test::initUI()
{
    setWindowModality(Qt::ApplicationModal);
    setAttribute(Qt::WA_DeleteOnClose);

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint);//无边框且最小化任务栏还原

    QPalette palette(this->palette());
    palette.setColor(QPalette::Background, Qt::white);
    this->setPalette(palette);//设置窗口背景颜色：白

    QPixmap minPix=style()->standardPixmap(QStyle::SP_TitleBarMinButton);
    QPixmap closePix=style()->standardPixmap(QStyle::SP_TitleBarCloseButton);
    ui->buttonMin->setIcon(minPix);
    ui->buttonClose->setIcon(closePix);//获取并设置

    ui->currentTimeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd \nhh:mm:ss dddd"));
    ui->usernameLabel->setText(QString::fromStdString(myUser.getName()));
    ui->submitButton->setDisabled(true);

    //提示用户开始测试
    initLabel = new QLabel(ui->scrollArea);
    initLabel->setText(tr("请点击开始测试按钮开始测试！"));
    initLabel->setGeometry(150, 100, 390, 50);
    QFont ft;
    ft.setPointSize(20);
    ft.setFamily(tr("楷体"));
    initLabel->setFont(ft);
    QPalette pa;
    pa.setColor(QPalette::WindowText, Qt::red);
    //初始化开始测试按钮
    initLabel->setPalette(pa);
    startButton = new QPushButton(ui->scrollArea);
    startButton->setGeometry(280, 180, 110, 30);
    startButton->setText(tr("开始测试"));
    ft.setPointSize(12);
    startButton->setFont(ft);
}

//初始化
void Test::init()
{
    mMove=false;//mouse moving
    pass = false;
    currentTid = "";
    currentDomain = "";
    limitScore = 0;

    openDatabase();
    QSqlQuery query(db);
    _first = currentKid.left(1);//根据当前知识节点查询数据库获得该知识节点的所需测试时间
    if (_first == "B")
    {
        query.exec("select title,domain from bk where bid='" + currentKid + "'");
    }
    else
    {
        query.exec("select title,domain from pk where pid='" + currentKid + "'");
    }
    while (query.next())
    {
        ui->currentPointNameLabel->setText(query.value(0).toString());
        currentDomain = query.value(1).toString();
    }
    query.exec("select * from test where kid='" + currentKid + "'");
    while (query.next())
    {
        currentTid = query.value(1).toString();
        QPalette pa;
        pa.setColor(QPalette::WindowText, Qt::red);
        ui->testSecondLabel->setText(query.value(2).toString());
        ui->testSecondLabel->setPalette(pa);
        ui->testMinuteLabel->setText(query.value(3).toString());
        ui->testMinuteLabel->setPalette(pa);
        ui->restSecondLabel->setText(query.value(2).toString());
        ui->restSecondLabel->setPalette(pa);
        ui->restMinuteLabel->setText(query.value(3).toString());
        ui->restMinuteLabel->setPalette(pa);
        limitScore = query.value(4).toInt();
    }

    timer1->start(1000);
}

//重写鼠标函数实现窗口自由移动
void Test::mousePressEvent(QMouseEvent *event)
{
    mMove = true;
    //记录下鼠标相对于窗口的位置
    //event->globalPos()鼠标按下时，鼠标相对于整个屏幕位置
    //pos() this->pos()鼠标按下时，窗口相对于整个屏幕位置
    mPos = event->globalPos() - pos();
    return QWidget::mousePressEvent(event);
}

void Test::mouseMoveEvent(QMouseEvent *event)
{
    //(event->buttons() && Qt::LeftButton)按下是左键
    //通过事件event->globalPos()知道鼠标坐标，鼠标坐标减去鼠标相对于窗口位置，就是窗口在整个屏幕的坐标
    if (mMove && (event->buttons() && Qt::LeftButton)
            && (event->globalPos()-mPos).manhattanLength() > QApplication::startDragDistance())
    {
        move(event->globalPos()-mPos);
        mPos = event->globalPos() - pos();
    }
    return QWidget::mouseMoveEvent(event);
}

void Test::mouseReleaseEvent(QMouseEvent* /* event */)
{
    mMove = false;
}
//mouse END

void Test::openDatabase()
{
    this->db = QSqlDatabase::addDatabase("QMYSQL", "test");
    this->db.setHostName("localhost");
    this->db.setUserName("root");
    this->db.setPassword("1234");
    this->db.setDatabaseName("knowledge");
    bool ok = this->db.open();
    if (!ok)
    {
        qcout << "Failed to connect database login!";
    }
    else
    {
        qcout << "Success!";
    }
}

//显示系统时间
void Test::timeUpdateSlot()
{
    QDateTime time = QDateTime::currentDateTime();
    ui->currentTimeLabel->setText(time.toString("yyyy-MM-dd \nhh:mm:ss dddd"));
}

//更新当前测试剩余时间
void Test::restTimeUpdateSlot()
{
    qint64 secs = startTestTime.secsTo(QDateTime::currentDateTime());

    qcout << secs;

    int minT = secs / 60;
    int secT = secs % 60;

    int restMinuteTime = 45;
    int restSecondTime = 0;

    if(secT > 0)
    {
        --restMinuteTime;
        restSecondTime = 60 - secT;
    }
    restMinuteTime -= minT;

    ui->restMinuteLabel->setText(QString::number(restMinuteTime));
    ui->restSecondLabel->setText(QString::number(restSecondTime));
}

//动态显示测试界面
void Test::startTestSlot()
{
    if (initLabel != nullptr)
    {
        delete initLabel;
        initLabel = nullptr;
    }
    if (startButton != nullptr)
    {
        delete startButton;
        startButton = nullptr;
    }

    startTestTime = QDateTime::currentDateTime();
    timer2->start(1000);
    connect(timer2, &QTimer::timeout, this, &Test::restTimeUpdateSlot);

    ui->submitButton->setDisabled(false);
    //ui->scrollArea->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    //ui->scrollArea->setWidgetResizable(true);
    allLayout = new QVBoxLayout;
    firstLayout = new QHBoxLayout;
    firstTitleLabel = new QLabel;
    firstTitleLabel->setText(tr("一、单项选择题"));
    firstLayout->addWidget(firstTitleLabel);
    firstLayout->addStretch();
    //ui->scrollArea->setLayout(firstLayout);
    allLayout->addLayout(firstLayout);
    int numRows;                //记录查询的行数
    QSqlQuery query(db);
    query.exec("select * from testcase where tid='" + currentTid + "'");
    QSqlDatabase defaultDB = QSqlDatabase::database();
    if (defaultDB.driver()->hasFeature(QSqlDriver::QuerySize))
    {
        numRows = query.size();
    }
    else
    {
        // this can be very slow
        query.last();
        numRows = query.at() + 1;
    }
    qcout << numRows;
    while (query.next())
    {
        //题目标题
        int questionId = query.value(1).toInt();
        QHBoxLayout *contentLayout = new QHBoxLayout;
        hlayout_vec.push_back(contentLayout);
        QString _question = query.value(1).toString() + "." + query.value(2).toString();
        QLabel *questionLabel = new QLabel;
        label_vec.push_back(questionLabel);
        //questionLabel->adjustSize();
        //questionLabel->setWordWrap(true);
        //questionLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

        questionLabel->setText(_question);
        questionLabel->setStyleSheet("QLabel{background:yellow}");
        contentLayout->addWidget(questionLabel);
        contentLayout->addStretch();
        //选项a
        QHBoxLayout *aLayout = new QHBoxLayout;
        hlayout_vec.push_back(aLayout);
        QRadioButton *aRadioButton = new QRadioButton;
        radiobutton_vec.push_back(aRadioButton);
        aRadioButton->setText(query.value(4).toString());
        aLayout->addWidget(aRadioButton);
        aLayout->addStretch();
        //选项b
        QHBoxLayout *bLayout = new QHBoxLayout;
        hlayout_vec.push_back(bLayout);
        QRadioButton *bRadioButton = new QRadioButton;
        radiobutton_vec.push_back(bRadioButton);
        bRadioButton->setText(query.value(5).toString());
        bLayout->addWidget(bRadioButton);
        bLayout->addStretch();
        //选项c
        QHBoxLayout *cLayout = new QHBoxLayout;
        hlayout_vec.push_back(cLayout);
        QRadioButton *cRadioButton = new QRadioButton;
        radiobutton_vec.push_back(cRadioButton);
        cRadioButton->setText(query.value(6).toString());
        cLayout->addWidget(cRadioButton);
        cLayout->addStretch();
        //选项d
        QHBoxLayout *dLayout = new QHBoxLayout;
        hlayout_vec.push_back(dLayout);
        QRadioButton *dRadioButton = new QRadioButton;
        radiobutton_vec.push_back(dRadioButton);
        dRadioButton->setText(query.value(7).toString());
        dLayout->addWidget(dRadioButton);
        dLayout->addStretch();
        //整理全部布局
        QButtonGroup *choiceGroup = new QButtonGroup;
        buttongroup_vec.push_back(choiceGroup);
        choiceGroup->addButton(aRadioButton);
        choiceGroup->addButton(bRadioButton);
        choiceGroup->addButton(cRadioButton);
        choiceGroup->addButton(dRadioButton);
        choiceGroup->setId(aRadioButton, 1);
        choiceGroup->setId(bRadioButton, 2);
        choiceGroup->setId(cRadioButton, 3);
        choiceGroup->setId(dRadioButton, 4);
        test_map.insert(make_pair(questionId, choiceGroup));
        //question_vec.push_back(choiceGroup);
        allLayout->addLayout(contentLayout);
        allLayout->addLayout(aLayout);
        allLayout->addLayout(bLayout);
        allLayout->addLayout(cLayout);
        allLayout->addLayout(dLayout);
    }
    QWidget* allWidget = new QWidget(ui->scrollArea);
    allWidget->setLayout(allLayout);
    //allLayout->addSpacing(5000);
    ui->scrollArea->setWidget(allWidget);
}

//提交测试结果
void Test::submitTestSlot()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("提示"));
    msgBox.setText(tr("您确定要提交试卷吗？"));
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Ok);
    int ret = msgBox.exec();
    switch (ret)
    {
    case QMessageBox::Ok:
        break;
    case QMessageBox::Cancel:
        return;
    }

    // 停止剩余时间的计时
    timer2->stop();
    delete timer2;
    timer2 = nullptr;

    // 累计学生所得分数
    int score = 0;
    map<int, QButtonGroup*>::const_iterator it = test_map.begin();
    while (it != test_map.end())
    {
        int i = (it->second)->checkedId();
        QString _answer;
        switch (i)
        {
        case 1:
            _answer = "A";
            break;
        case 2:
            _answer = "B";
            break;
        case 3:
            _answer = "C";
            break;
        case 4:
            _answer = "D";
            break;
        }

        QSqlQuery query1(db);
        query1.prepare("select answer,score from testcase where tid=:tid and qid=:qid");
        query1.bindValue(":tid", currentTid);
        query1.bindValue(":qid", it->first);
        query1.exec();
        while (query1.next())
        {
            if (query1.value(0).toString() == _answer)
            {
                //加分
                score += query1.value(1).toInt();
            }
        }
        ++it;
    }

    QSqlQuery query(db);
    query.prepare("select score from testresult where sid=:sid and kid=:kid and tid=:tid");
    query.bindValue(":sid", myUser.getSid());
    query.bindValue(":kid", currentKid);
    query.bindValue(":tid", currentTid);
    if(!query.exec())
    {
       qcout << query.lastError();
    }
    else
    {
        qcout << "Sql executes sucessfully!";
        // 之前测试，存在测试结果记录，更新最新分数
        if(query.next())
        {
            QSqlQuery query1(db);
            query1.prepare("update testresult set score=:score where sid=:sid and kid=:kid and tid=:tid");
            query1.bindValue(":score", score);
            query1.bindValue(":sid", myUser.getSid());
            query1.bindValue(":kid", currentKid);
            query1.bindValue(":tid", currentTid);
            if(!query1.exec())
            {
               qcout << query1.lastError();
            }
            else
            {
                qcout << "Sql executes sucessfully!";
            }

        }
        // 没有测试过，插入最新测试结果
        else
        {
            //在测试结果表中插入用户测试记录
            QSqlQuery query1(db);
            query1.prepare("insert into testresult(sid,kid,tid,score) values(:sid,:kid,:tid,:score)");
            query1.bindValue(":sid", myUser.getSid());
            query1.bindValue(":kid", currentKid);
            query1.bindValue(":tid", currentTid);
            query1.bindValue(":score", score);
            if(!query1.exec())
            {
               qcout << query1.lastError();
            }
            else
            {
                qcout << "Sql executes sucessfully!";
            }
        }
    }



    //    //更新学习行为记录表中当前用户的最后一条学习记录，修改通过情况
    //    QString beginTime;
    //    query.prepare("select begin from behavior where sid=:sid and kid=:kid");
    //    query.bindValue(":sid", myUser.getSid());
    //    query.bindValue(":kid", currentKid);
    //    query.exec();
    //    while (query.next())
    //    {
    //        query.last();
    //        beginTime = query.value(0).toString();
    //        qcout << score;
    //    }

    if (score >= limitScore)
    {
        pass = true;
        //更新推荐路径表
        query.prepare("update recpath set state=1 where sid=:sid and kid=:kid");
        query.bindValue(":sid", myUser.getSid());
        query.bindValue(":kid", currentKid);
        if(!query.exec())
        {
           qcout << query.lastError();
        }
        else
        {
            qcout << "Sql executes sucessfully!";
        }

        qcout << myUser.getSid();
        qcout << currentKid;

        //更新学习行为记录表中当前用户的最后一条学习记录，修改通过情况
        query.prepare("insert into behavior(sid,kid,cid,begin,end,pass,note) values(:sid,:kid,:cid,:begin,:end,:pass,:note)");
        query.bindValue(":sid", myUser.getSid());
        query.bindValue(":kid", currentKid);
        query.bindValue(":cid", currentCid);
        query.bindValue(":begin", startTestTime.toString("yyyy-MM-dd hh:mm:ss"));
        query.bindValue(":end", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        query.bindValue(":pass", 1);
        query.bindValue(":note", tr("学习并通过测试用例"));
        if(!query.exec())
        {
           qcout << query.lastError();
        }
        else
        {
            qcout << "Sql executes sucessfully!";
        }
        //        query.prepare("update behavior set pass=1 where sid=:sid and kid=:kid and begin=:begin");
        //        query.bindValue(":sid", myUser.getSid());
        //        query.bindValue(":kid", currentKid);
        //        query.bindValue(":begin", beginTime);
        //        query.exec();

        //更新学习路径记录表
        query.prepare("insert into path(sid,domain,kid,begintime,score) values(:sid,:domain,:kid,:begintime,:score)");
        query.bindValue(":sid", myUser.getSid());
        query.bindValue(":domain", currentDomain);
        query.bindValue(":kid", currentKid);
        query.bindValue(":begintime", startTestTime.toString("yyyy-MM-dd hh:mm:ss"));
        query.bindValue(":score", score);
        if(!query.exec())
        {
           qcout << query.lastError();
        }
        else
        {
            qcout << "Sql executes sucessfully!";
        }
    }
    else
    {
        pass = false;
        //更新推荐路径表
        query.prepare("update recpath set state=0 where sid=:sid and kid=:kid");
        query.bindValue(":sid", myUser.getSid());
        query.bindValue(":kid", currentKid);
        query.exec();

        //        if (_first == "P")
        //        {
        //            //更新推荐路径表
        //            query.prepare("update recpath set state=1 where sid=:sid and kid=:kid");
        //            query.bindValue(":sid", myUser.getSid());
        //            query.bindValue(":kid", currentKid);
        //            query.exec();
        //        }

        //更新学习行为记录表中当前用户的最后一条学习记录，修改通过情况
        //        query.prepare("update behavior set pass=0 where sid=:sid and kid=:kid and begin=:begin");
        //        query.bindValue(":sid", myUser.getSid());
        //        query.bindValue(":kid", currentKid);
        //        query.bindValue(":begin", beginTime);
        //        query.exec();

        query.prepare("insert into behavior(sid,kid,cid,begin,end,pass,note) values(:sid,:kid,:cid,:begin,:end,:pass,:note)");
        query.bindValue(":sid", myUser.getSid());
        query.bindValue(":kid", currentKid);
        query.bindValue(":cid", currentCid);
        query.bindValue(":begin", startTestTime.toString("yyyy-MM-dd hh:mm:ss"));
        query.bindValue(":end", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        query.bindValue(":pass", 1);
        query.bindValue(":note", tr("正在学习知识点"));
    }



    QMessageBox::information(this, tr("恭喜"), tr("您已经提交成功！"));
    ui->submitButton->setEnabled(false);
    //释放空间
    if (firstLayout != nullptr && firstTitleLabel != nullptr)
    {
        delete firstLayout;
        firstLayout = nullptr;
        delete firstTitleLabel;
        firstTitleLabel = nullptr;
    }
    for (vector<QHBoxLayout*>::iterator it = hlayout_vec.begin();
         it != hlayout_vec.end(); ++it)
    {
        if ((*it) != nullptr)
        {
            delete *it;
            (*it) = nullptr;
        }
    }
    for (vector<QLabel*>::iterator it = label_vec.begin();
         it != label_vec.end(); ++it)
    {
        if ((*it) != nullptr)
        {
            delete *it;
            (*it) = nullptr;
        }
    }
    for (vector<QRadioButton*>::iterator it = radiobutton_vec.begin();
         it != radiobutton_vec.end(); ++it)
    {
        if ((*it) != nullptr)
        {
            delete *it;
            (*it) = nullptr;
        }
    }
    for (vector<QButtonGroup*>::iterator it = buttongroup_vec.begin();
         it != buttongroup_vec.end(); ++it)
    {
        if ((*it) != nullptr)
        {
            delete *it;
            (*it) = nullptr;
        }
    }
    if (allLayout != nullptr)
    {
        delete allLayout;
        allLayout = nullptr;
    }

    allLayout = new QVBoxLayout;
    QWidget *w = new QWidget;
    QLabel *endLabel = new QLabel(w);


    if (pass)
    {//如果通过测试
        endLabel->setText(tr("本次考试已经结束，您的分数为： ") + QString::number(score) + tr(" ,恭喜您已通过测试，您要继续学习吗？"));
        QPushButton *nextButton = new QPushButton(w);
        nextButton->setText(tr("下一知识点"));
        nextButton->setGeometry(280, 180, 110, 30);
        connect(nextButton, &QPushButton::clicked, this, &Test::nextKnowledgeSlot);
    }
    else
    {//没有通过测试
        endLabel->setText(tr("本次考试已经结束，您的分数为： ") + QString::number(score) + tr(" ,您没有通过测试，请重新学习"));
        QPushButton *againButton = new QPushButton(w);
        againButton->setText(tr("返回教学模块"));
        againButton->setGeometry(280, 180, 110, 30);
        if (_first == "B")
        {
            connect(againButton, &QPushButton::clicked, this, &Test::againKnowledgeSlot);
        }
        else
        {
            connect(againButton, &QPushButton::clicked, this, &Test::nextKnowledgeSlot);
        }
    }
    endLabel->setGeometry(100, 100, 500, 50);
    allLayout->addWidget(w);
    ui->scrollArea->setLayout(allLayout);
}

//下一个知识节点
void Test::nextKnowledgeSlot()
{
    beforeKid = currentKid;
    QSqlQuery query(db);
    query.prepare("select kid from recpath where sid=:sid and state=0 order by orders");
    query.bindValue(":sid", myUser.getSid());
    if(!query.exec())
    {
       qcout << query.lastError();
    }
    else
    {
        qcout << "Sql executes sucessfully!";
        if (query.next())
        {
            currentKid = query.value(0).toString();
            qcout << currentKid;
        }
        else
        {
            QMessageBox::information(this, tr("教学系统"), tr("恭喜你，成功学完所有知识点！"));
        }
    }

//    if (_first=="P")
//    {
//        query.prepare("update recpath set state=0 where sid=:sid and kid=:kid");
//        query.bindValue(":sid", myUser.getSid());
//        query.bindValue(":kid", beforeKid);
//        query.exec();
//    }
    this->close();
}

//重新学习当前知识点
void Test::againKnowledgeSlot()
{
    this->close();
}
