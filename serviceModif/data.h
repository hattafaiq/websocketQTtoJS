#ifndef DATA_H
#define DATA_H

#include <QUdpSocket>
#include <QTimer>
#include <QObject>
#include <QList>
#include <QByteArray>
#include <QWebSocket>
#include <QTimer>
#include <QUdpSocket>
#include <QDate>
#include <QString>
#include "QWebSocketServer"
#include "setting.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSqlDatabase>
#include <threader.h>
#include "database.h"

class data : public QObject
{
    Q_OBJECT
public:
    QByteArray simpanan;
    explicit data(QObject *parent = nullptr);
    void req_UDP();
    virtual ~data();
    unsigned short spsX;
    int kirim;
    void set_memory();
    void free_memory();
    void init_setting(init_setting_k *Temp);
    void cek_settings(init_setting_k *Temp);
    //struct init_setting_k *Temp;

signals:

public slots:
    void readyReady(); //(QByteArray datagram);//data tidak mau masuk
    void init_time();
    void refresh_plot();
    void datamanagement();
    void datamanagement2();
    void start_database();

private slots:
    void onNewConnection();
    void processMessage(QByteArray message);
    void socketDisconnected();
    void showTime();
    void sendDataClient1(QByteArray isipesan);
    void sendDataClient2(QString isipesan2);


private:
    //
    struct init_setting_k *Temp;

    struct filepesan *yuhu;
    struct kirim kri,*kr;

    float dataku[256];
    char a;

    unsigned int panjang_buffer_waveform;
    unsigned int panjang_buffer_spektrum;
    int period_simpan;
    int spektrum_points;
    int paket_dikirim;
    float *data_save[JUM_PLOT];
    float *data_get[JUM_PLOT];
    float data_send[256];
    double *data_asend[JUM_PLOT];
    int cnt_ch[JUM_PLOT];
    int cnt_ch1;
    int cnt_ch2;
    int cnt_ch3;
    int cnt_ch4;
    int counterCH1;
    int counterCH2;
    int counterCH3;
    int counterCH4;
    int counterCH5;
    int counterCH6;
    int counterCH7;
    int counterCH8;
    ///
    int pernah_penuh;
    //timer
    QTimer *timer;
    QTimer *timera;
    QTimer *tim;

    //websocket
    QWebSocketServer *m_pWebSocketServer1;
    QList<QWebSocket *> CG_NewClient;
    QList<QWebSocket *> CG_PenSub;
    QList<QWebSocket *> Subcribe_wave1;
    QList<QWebSocket *> Subcribe_wave2;

   // QWebSocket *C_angbaru;
   // QWebSocket *UNSUB_Client;
    QWebSocket *ind_Client;
    QWebSocket *C_NewCon;

    QWebSocket *pClient1;
    QWebSocket *pClientkirim;

    QString *datas;
    //date
    QTimer *jam;
    QTimer *timers;
    QDate date;
    QString dateTimeText;
    QString time_text;
    //
    //parsing UDP
    QByteArray datagram;
    quint16 senderPort;
    QHostAddress sendera;
    QUdpSocket *socket;
    int tim_count;
    // inisial data
    double *data_y_voltage[JUM_PLOT];
    float data_y_voltage1[BESAR_PAKET_F];//256 data
    float data_y_voltage2[BESAR_PAKET_F];//256 data
    float data_y_voltage3[BESAR_PAKET_F];//256 data
    float data_y_voltage4[BESAR_PAKET_F];//256 data
    float data_y_voltage5[BESAR_PAKET_F];//256 data
    float data_y_voltage6[BESAR_PAKET_F];//256 data
    float data_y_voltage7[BESAR_PAKET_F];//256 data
    float data_y_voltage8[BESAR_PAKET_F];//256 data
    float data10paket_1[PAKET_10];
    float data10paket_2[PAKET_10];
    float data10paket_3[PAKET_10];
    float data10paket_4[PAKET_10];
    float data10paket_5[PAKET_10];
    float data10paket_6[PAKET_10];
    float data10paket_7[PAKET_10];
    float data10paket_8[PAKET_10];
    // SQL
//    QSqlDatabase m_db;
//    QString simpanaja;
//    static QList<QWebSocket*> plist;
    threader *threadku;
    database *dbase;
    int count_db;


};
#endif // DATA_H
