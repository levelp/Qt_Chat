/// Работа с сетью
/// ==============
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <QTime>

MainWindow::MainWindow(QWidget* parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow) {
  ui->setupUi(this);

  socket = NULL;

  //on_enterChatButton_clicked();


  ///<--
}

MainWindow::~MainWindow() {
  delete ui;
}

/// Создание UDP-чата
///-->
void MainWindow::UdpChat(QString nick, int port) {
  // Если соединение уже открыто, то закрываем его
  if(socket != NULL) {
    socket->close();
    delete socket;
    socket = NULL;
  }

  log(QString("Создание чата: port %1").arg(port));
  socket = new QUdpSocket(this);
  QHostAddress address = QHostAddress("192.168.2.5"); // - конкретный IP, с которого можно подключиться

  // QHostAddress::Any - принимать
  //   сообщения со всех IP адресов
  if(socket->bind(address/*QHostAddress::AnyIPv4*/, port)) {
    // При получении данных (сигнал readyRead)
    // вызываем метод (слот) read, который читает и обрабатывает сообщение
    connect(socket, SIGNAL(readyRead()), this, SLOT(read()));
  } else {
    // Какая-то программа на этом компьютере уже
    // заняла порт port
    qDebug() << "Port " << port << " in use. Change port!";
    log(QString("Port %1 in use. Change port!").arg(port));
    return;
  }

  send(nick + " - в чате", USUAL_MESSAGE);
  log(QString("Отвечаем своим ником: %1").arg(ui->nicknameEdit->text()));
  QTime now = QTime::currentTime();
  QString nowStr = now.toString("hh:mm:ss");
  QString str = nowStr + " " +
          ui->nicknameEdit->text();
  send(str,
       PERSON_ONLINE);

  //ui->onlineList->addItem(str);

  // Таймер опроса "кто онлайн"
  ///-->
  QTimer* timer = new QTimer(this);
  // Соединяем сигнал со слотом
  connect(timer,
          SIGNAL(timeout()),
          this,
          SLOT(refreshOnlineList()));
  timer->start(15000);
}
///<--

/// Нажимаем на кнопку "Войти в чат"
///-->
void MainWindow::on_enterChatButton_clicked() {
  QString nick = ui->nicknameEdit->text();
  UdpChat(nick,
          ui->portNumEdit->text().toInt());
  // Разрешаем отправлять сообщения только когда уже в чате
  ui->sendButton->setEnabled(true);
  ui->nicknameEdit->setEnabled(false);
}
///<--


/// Отправка сообщения в сеть
///-->
void MainWindow::send(QString str, MessageType type) {
  if(type == USUAL_MESSAGE)
    log(QString("send: %1 %2").arg(str).arg(type));

  // Полный пакет данных будет в массиве data
  QByteArray data; // Массив данных для отправки

  // Последовательно выводим в него байты
  QDataStream out(&data, QIODevice::WriteOnly);
  out << qint8(type); // Тип сообщения
  out << str; // Само сообщение

  // Отправляем полученный массив данных всем в локальный сети
  // на порт указанный в интерфейсе
  socket->writeDatagram(data,
                        QHostAddress::Broadcast,
                        ui->portNumEdit->text().toInt() );

}
///<--

/// Получение сообщения по UDP
///-->
void MainWindow::saveToLogFile(QString str) {
  QFile file("log.txt");
  file.open(QIODevice::WriteOnly | QIODevice::Text
            | QIODevice::Append );
  QTextStream log(&file);
  log.setCodec("UTF-8");
  log << str << endl;
  file.close();
}

void MainWindow::read() {
  while (socket->hasPendingDatagrams()) {
    log("read>>");
    // Массив (буфер) для полученных данных
    QByteArray buf;
    // Устанавливаем массиву размер
    // соответствующий размеру полученного пакета данных
    buf.resize(socket->pendingDatagramSize());
    QHostAddress* address = new QHostAddress();
    socket->readDatagram(buf.data(), buf.size(), address);
    log(QString("Message from IP: %1 size: %2").arg(address->toString()).arg(buf.size()));

    // Разбор полученного пакета
    QDataStream in(&buf, QIODevice::ReadOnly);

    // Получаем тип пакета
    qint8 type = 0;
    in >> type;

    QString str;
    in >> str;
    log(QString("read>> %1 %2").arg(str).arg(type));

    if(str.length() == 0)
      return;

    if (type == USUAL_MESSAGE) {
      // Записываем входящие сообщения в файл
      saveToLogFile(str);

      // Отображаем строчку в интерфейсе
      ui->plainTextEdit->appendPlainText(str);
    } else if (type == PERSON_ONLINE) {
      // Добавление пользователя с считанным QHostAddress
      //QStringList list = str.split(" ");
      //QString timeStr = list.at(0);
      // Время выделили, дальше вырезаем
      // из строки ник.
      // Ищем в списке, если есть => обновляем
      // Если нет, добавляем.
      //QString nick = str.right(timeStr.length());
//      if(ui->onlineList->count())
//      {
//          for(int i = 0; i < ui->onlineList->count();i++)
//          {

//              QListWidgetItem* item = ui->onlineList->item(i);
//              //
//              QString lstStr = item->text();
//              QStringList lstList = lstStr.split(" ");
//              QString dateStr = lstList.at(0);

//              QString nickLst = lstStr.right(dateStr.length());

//              if(nickLst != nick)
//                ui->onlineList->addItem(str);
//          }
//      }
//      else
//      {
        ui->onlineList->addItem(str);
//      }
    } else if (type == WHO_IS_ONLINE) {
      log(QString("Отвечаем своим ником: %1").arg(ui->nicknameEdit->text()));
      QTime now = QTime::currentTime();
      QString nowStr = now.toString("hh:mm:ss");
      send(nowStr + " " +
           ui->nicknameEdit->text(),
           PERSON_ONLINE);
    }
  }
}
///<--

/// Нажимаем на кнопку "Отправить сообщение"
///-->
void MainWindow::on_sendButton_clicked() {
  // Вся строка сообщения: "ник: сообщение"
  send(ui->nicknameEdit->text() + ": " +
       ui->messageEdit->text(),
       USUAL_MESSAGE);

  ui->messageEdit->clear();
  QTime now = QTime::currentTime();
  QString nowStr = now.toString("hh:mm:ss");
  send(nowStr + " " +
       ui->nicknameEdit->text(),
       PERSON_ONLINE);

}
///<--

void MainWindow::on_messageEdit_returnPressed() {
  on_sendButton_clicked();
}

/// Обновляем список тех кто online
///-->
void MainWindow::refreshOnlineList() {
  // TODO: хранить время, когда последний раз этот человек был в сети

  // Удаляем тех, от кого давно не было сообщений
  for(int i = 0; i < ui->onlineList->count(); ++i) {
    // Получаем i-ую запись из списка
    QListWidgetItem* item = ui->onlineList->item(i);
    //
    QString str = item->text();
    QStringList list = str.split(" ");
    QString dateStr = list.at(0);

    //QString nickLst = str.right(dateStr.length());

    QTime time = QTime::fromString(dateStr, "hh:mm:ss");
    QTime now = QTime::currentTime();
    int diff = time.msecsTo(now);

    //log(QString("%1 %2 %3").arg(dateStr).arg(now).arg(diff));

    //QString nick = ui->nicknameEdit->text();

    //log(QString("nicks%1%2").arg(nickLst).arg(nick));
    // Удаляем запись из списка
    if(diff > 10000){
      //if(nickLst != nick){
          ui->onlineList->takeItem(i);
      //}
    }
  }

  //send("Who is online?", WHO_IS_ONLINE);
}
///<--

// Запись в log-окно
void MainWindow::log(QString s) {
  ui->log->append(s);
}
