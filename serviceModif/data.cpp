#include "data.h"
#include "setting.h"

extern struct d_global global;
extern struct init_setting_k Temp;

data::data(QObject *parent) : QObject(parent)
{
  threadku = new threader();
  spektrum_points = 2.56 * 800;//line nya 200 = 512 data // line nya 800 = 2048 data
  panjang_buffer_waveform= 10 * 10240 ;
  period_simpan = 1000;
  paket_dikirim= 256;//PAKET_10;
  panjang_buffer_spektrum = MAX_FFT_POINT;
  //report 2 detik aman selama 2 jam
  //perlu validasi ketika membaca data, jika 1-4 tidak ada dan dimulai
  //dari data 5 atau seterusnya
  //
    Temp = new init_setting_k;
        QFile input("setting.ini");
        if(input.exists())
        {
           cek_settings(Temp);
        }
        else
        {
           init_setting(Temp);
        }
    yuhu = new filepesan;
    count_db = 1;
    pernah_penuh = 0;
    //INIT_udp
    socket = new QUdpSocket(this);
    socket->bind(QHostAddress::Any, 5008);
    connect(socket, SIGNAL(readyRead()), this, SLOT(readyReady()));
    //INIT_websocket
    m_pWebSocketServer1 = new QWebSocketServer(QStringLiteral("OVM"),QWebSocketServer::NonSecureMode,this);
    m_pWebSocketServer1->listen(QHostAddress::Any, 1234);
    connect(m_pWebSocketServer1, SIGNAL(newConnection()),this, SLOT(onNewConnection()));
    //INIT_database
    dbase->check_db_exist("ovm_dbe",count_db);
    set_memory();

    for(int i =0; i<JUM_PLOT;i++)
    {
    cnt_ch[i] = 0;
    }
    cnt_ch1 = 0;
    cnt_ch2 = 0;
    cnt_ch3 = 0;
    cnt_ch4 = 0;
}

data::~data()
{
    m_pWebSocketServer1->close();
    qDeleteAll(Subcribe_wave1.begin(), Subcribe_wave1.end());//paket10
    qDeleteAll(Subcribe_wave2.begin(), Subcribe_wave2.end());//paket10
    delete threadku;
  //  delete Temp;
    free_memory();
}
void data::free_memory()
{
    for (int i = 0; i < JUM_PLOT; i++)
    {
        free(data_save[i]);
        free(data_get[i]);
      //  free(data_send[i]);
        free(data_asend[i]);
    }
}
void data::cek_settings(init_setting_k *Temp)
{
    QString pth = "setting.ini";
    QSettings settings(pth, QSettings::IniFormat);
//    Temp->modulIP1k = settings.value("IP1");
//    Temp->modulIP2k = settings.value("IP2");
    Temp->fmax = settings.value("Fmax").toInt() ;
    Temp->timerdbk = settings.value("IntervalDB").toInt();
    Temp->line_dbSpect = settings.value("LineDB_spec").toInt();

    qDebug() << "==============================<>";
//    qDebug() << Temp->modulIP1k << " data ip1";
//    qDebug() << Temp->modulIP2k << " data ip1";
    qDebug() << Temp->fmax << " data Fmax";
    qDebug() << Temp->timerdbk<< " data Interval DB";
    qDebug() << Temp->line_dbSpect<< "len data spect";
    qDebug() << "==============================<>";
}

void data::init_setting(init_setting_k *Temp)
{
#if 0
    QString pth = "setting.ini";
    QFile output(pth);
    QByteArray BA((char *) &Temp, sizeof (struct init_setting_k));
    if (output.open(QIODevice::WriteOnly)) {
        output.write(BA);
        output.close();
    }
#endif
#if 1
//    QSettings settings("TestQSettings.ini", QSettings::IniFormat);
//    settings.setValue("AAA",QString("111"));
//    settings.setValue("BBB",222);

    QString pth = "setting.ini";
    QSettings settings(pth, QSettings::IniFormat);
    qDebug()<<"tulis";

    memset((char *) Temp, 0, sizeof (struct init_setting_k));
//    Temp->modulIP1k = "192.168.0.101";
//    Temp->modulIP2k = "192.168.0.102";
    Temp->line_dbSpect = 800;
    Temp->fmax= 1000;
   // Temp->timerdbk =1000;

//    settings.setValue("IP1",Temp->modulIP1k);
//    settings.setValue("IP2",Temp->modulIP2k);
    settings.setValue("LineDB_spec",Temp->line_dbSpect);
    settings.setValue("Fmax",Temp->fmax);
    settings.setValue("IntervalDB",Temp->timerdbk);

    qDebug()<<"selesai tulis";
#endif
}

void data::set_memory()
{
    for (int i=0; i<JUM_PLOT; i++)
    {
        data_save[i] = (float *) malloc(((20480 + BESAR_PAKET) * 4) * sizeof(float));
        memset( (char *) data_save[i], 0, (((20480 + BESAR_PAKET) * 4) * sizeof(float)));
        data_get[i] = (float *) malloc(16540 * sizeof(float));
        memset( (char *) data_get[i], 0, 16540 * sizeof(float));

        data_asend[i] = (double *) malloc((paket_dikirim*2) * sizeof(double));
        memset( (char *) data_asend[i], 0, ((paket_dikirim*2) * sizeof(double)));
//        data_send[i] = (float *) malloc((paket_dikirim) * sizeof(float));
//        memset( (char *) data_send[i], 0, ((paket_dikirim) * sizeof(float)));

    }
}


void data::init_time()
{
    timer = new QTimer(this);
    timera = new QTimer(this);
   // tim = new QTimer(this);
    QObject::connect(timer,SIGNAL(timeout()),this, SLOT(refresh_plot()));
    QObject::connect(timera,SIGNAL(timeout()),this, SLOT(start_database()));
   // QObject::connect(tim,SIGNAL(timeout()),this, SLOT(datamanagement()));
    timer->start(TIME_REQ);//2200 // harus sesuai interval berdasarkan sps
    timera->start(period_simpan);// aman di 2 detik selama 2 jam
   // tim->start(1000);
}

void data::req_UDP()
{
    QByteArray Data;
    Data.append("getdata");
    socket->writeDatagram(Data,QHostAddress("192.168.0.101"), 5006);
    socket->writeDatagram(Data,QHostAddress("192.168.0.102"), 5006);
    //kirim=0;
}

void data::showTime()
{
    QTime time = QTime::currentTime();
    time_text = time.toString("hh:mm:ss:z");
}


void data::readyReady()
{
    struct tt_req2 *p_req2;
    float *p_data;
    int i_kanal;

    //dataku = yuhu->array;

    while (socket->hasPendingDatagrams())
    {

        datagram.resize(socket->pendingDatagramSize());
        socket->readDatagram(datagram.data(), datagram.size(), &sendera, &senderPort);
        //QHostAddress modul1_ip("192.168.0.101");
        QString ip1 =QString::fromUtf8("192.168.0.101");
        QString ip2 =QString::fromUtf8("192.168.0.102");
        QHostAddress ip_modul_1, ip_modul_2;
        ip_modul_1.setAddress(ip1);
        ip_modul_2.setAddress(ip2);
         p_req2 = (struct tt_req2 *) datagram.data();
         p_data = (float *) p_req2->buf;
         i_kanal = p_req2->cur_kanal;
         spsX = p_req2->sps;

         int no_module = -1;

         if(sendera.toIPv4Address() == ip_modul_1.toIPv4Address())
         {
             //qDebug()<<" Data dari modul 1 | kanal"<<i_kanal+1;
             no_module = 0;
         }
         else if(sendera.toIPv4Address() == ip_modul_2.toIPv4Address())
         {
             i_kanal = i_kanal+4;
             //qDebug()<<" Data dari modul 2 | kanal"<<i_kanal+1;
             no_module = 1;
         }

         if(no_module == -1){ return;}

         if(i_kanal>=JUM_PLOT){return;}



             if((no_module==0)||(no_module==1)) //ip pertama
             {
                 for (int i=0; i<BESAR_PAKET_F; i++)
                 {
                     //data_y_voltage[i_kanal][i + ((2560 % 10) * BESAR_PAKET_F)] = (double) p_data[i];
                     cnt_ch[i_kanal]++;
                     data_save[i_kanal][cnt_ch[i_kanal]] = p_data[i];
                   //  data_send[i_kanal][cnt_ch[i_kanal]] = (double) p_data[i];
                 }
                // datamanagement();


                // qDebug()<<"ip pertama";
                 // ikut 1 paket
                 if(i_kanal==0)//kanal 1
                 {
                     //qDebug()<<"-KANAL  1-" <<"ip pertama";
                     counterCH1++;
                     //qDebug()<<"paket ke "<<counterCH1;
                     if (counterCH1<11)
                     {
                    //mengirim 1o paket kanal 1
                         for(int i=0; i<256; i++)
                             {
                           //  yuhu->array[i] = p_data[i];
                                data_send[i]= p_data[i]; //mengirim 1 paket
                                data10paket_1[(counterCH1-1)*256+i]=p_data[i];//tracking data dari 0-2560 per paket data sebanyak 256
                             }
                       }//counter
                     if(counterCH1==10)counterCH1=0;


                 }//kanal 1

                 else if(i_kanal==1)//kanal 2
                 {
                    // qDebug()<<"-KANAL  2-" <<"ip pertama";
                     counterCH2++;
                     if (counterCH2<11)
                     {
                    //mengirim 1o paket kanal 2
                         for(int i=0; i<256; i++)
                             {
//                                data_y_voltage2[i]=p_data[i%256]; //mengirim 1 paket
                                data10paket_2[(counterCH2-1)*256+i]=p_data[i%256];//tracking data dari 0-2560 per paket data sebanyak 256

                             }
                     }
                     if(counterCH2==10) counterCH2=0;
                 }//kanal 2
                 else if(i_kanal==2)//kanal 3
                 {
                   //  qDebug()<<"-KANAL  3-" <<"ip pertama";
                     counterCH3++;
                     if (counterCH3<11)
                     {
                    //mengirim 1o paket kanal 3
                         for(int i=0; i<256; i++)
                             {
//                                data_y_voltage3[i]=p_data[i%256]; //mengirim 1 paket
                                data10paket_3[(counterCH3-1)*256+i]=p_data[i%256];//tracking data dari 0-2560 per paket data sebanyak 256
                             }
                     }
                     if(counterCH3==10) counterCH3=0;
                 }// kanal 3
                 else if(i_kanal==3)//kanal 3
                 {
                    // qDebug()<<"-KANAL  4-" <<"ip pertama";
                     counterCH4++;
                     if (counterCH4<11)
                     {
                    //mengirim 1o paket kanal 4
                         for(int i=0; i<256; i++)
                             {
//                                data_y_voltage4[i]=p_data[i%256]; //mengirim 1 paket
                                data10paket_4[(counterCH4-1)*256+i]=p_data[i%256];//tracking data dari 0-2560 per paket data sebanyak 256
                             }//for loop
                     }//counter channel
                     if(counterCH4==10)//counterCH4=0;
                     {
                        counterCH4=0;
                        datamanagement();
                     }
                 }//kanal 4
//if(counterCH4==10)datamanagement();
 //           }//ip
            // datamanagement();
#if 0
///-----------------------------------------batas sortir IP------------------------------------------------//
//             else if(sendera.toIPv4Address() == ip_modul_2.toIPv4Address()) //ip kedua
//             {
                 //qDebug()<<"ip kedua";
                 //counterCH5++;// ikut 1 paket
                 else if(i_kanal==4)//kanal 5
                 {
                     //qDebug()<<"-KANAL  5-" <<"ip kedua";
                     counterCH5++;
                     if (counterCH5<11)
                     {
                    //mengirim 1o paket kanal 5
                         int i;
                         for(i=0; i<256; i++)
                             {
                                // data_y_voltage1[i]=p_data[i%256]; //mengirim 1 paket
                                data10paket_5[(counterCH5-1)*256+i]=p_data[i%256];//tracking data dari 0-2560 per paket data sebanyak 256
                             //   qDebug()<<data10paket_5[(counterCH5-1)*256+i];
                             }
                                // qDebug()<<"oke masuk ke " << counterCH1 << "CH1" << "-jumlah data " << (counterCH1-1)*256+i;

                     }
                     if(counterCH5==10)counterCH5=0;
                 }//kanal 5
                 else if(i_kanal==5)//kanal 6
                 {
                   //  qDebug()<<"-KANAL  6-"<<"ip kedua";
                     counterCH6++;
                     if (counterCH6<11)
                     {
                    //mengirim 1o paket kanal 6
                         int i;
                         for(i=0; i<256; i++)
                             {
                                // data_y_voltage1[i]=p_data[i%256]; //mengirim 1 paket
                                data10paket_6[(counterCH6-1)*256+i]=p_data[i%256];//tracking data dari 0-2560 per paket data sebanyak 256
                             }
                     }
                     if(counterCH6==10)counterCH6=0;
                 }//kanal 6
                 else if(i_kanal==6)//kanal 7
                 {
                    // qDebug()<<"-KANAL  7-"<<"ip kedua";
                     counterCH7++;
                     if (counterCH7<11)
                     {
                    //mengirim 1o paket kanal 7
                         int i;
                         for(i=0; i<256; i++)
                             {
                                // data_y_voltage1[i]=p_data[i%256]; //mengirim 1 paket
                                data10paket_7[(counterCH7-1)*256+i]=p_data[i%256];//tracking data dari 0-2560 per paket data sebanyak 256
                             }
                                // qDebug()<<"oke masuk ke " << counterCH1 << "CH1" << "-jumlah data " << (counterCH1-1)*256+i;
                     }
                     if(counterCH7==10)counterCH7=0;
                 }// kanal 7
                 else if(i_kanal==7)//kanal 8
                 {
                   //  qDebug()<<"-KANAL  8-"<<"ip kedua";
                     counterCH8++;
                  //   qDebug()<<"paket ke "<<counterCH8;
                     if (counterCH8<11)
                     {
                    //mengirim 1o paket kanal 8
                         int i;
                         for(i=0; i<256; i++)
                             {
                                // data_y_voltage1[i]=p_data[i%256]; //mengirim 1 paket
                                data10paket_8[(counterCH8-1)*256+i]=p_data[i%256];//tracking data dari 0-2560 per paket data sebanyak 256
                             }//for loop
                              // qDebug()<<"oke masuk ke " << counterCH1 << "CH1" << "-jumlah data " << (counterCH1-1)*256+i;
                     }//counter channel
                     if(counterCH8==10)
                     {
                         counterCH8=0;
                         //datamanagement();
                     }
                 }//kanal 8
#endif
            }//ip KEDUA
//             QByteArray ba11;
//             QByteArray ba21;
//             QString wave11 ="wave1";
//               //QString qs = "String";
//               ba11 += wave11;
//             qDebug()<<ba11<<"   code -------------------->";
        }// while
    //start_database();
}//void


void data::datamanagement()
{
#if 0
    QJsonObject isikanal;
    //kanal_p1
    QJsonObject kanalip1;
    QJsonObject ip1;

    QJsonArray buff1;
    QJsonArray buff2;
    QJsonArray buff3;
    QJsonArray buff4;
    QJsonValue spsip1_;

    for(int i=0; i<2560; i++)
    {
        //buff[a].push_back(data_y_voltage[a][i]);
        buff1.push_back(data10paket_1[i]);
      //  buff2.push_back(data10paket_2[i]);
     //  buff3.push_back(data10paket_3[i]);
      // buff4.push_back(data10paket_4[i]);
    }
    //start_database();

    kanalip1.insert("kanal1",buff1);
   // kanalip1.insert("kanal2",buff2);
   // kanalip1.insert("kanal3",buff3);
   // kanalip1.insert("kanal4",buff4);
   // kanalip1.insert("spsip1",spsX);

    ip1.insert("ip1",kanalip1);
    QJsonDocument paket1Bro(ip1);
    QString sout = QString::fromUtf8(paket1Bro.toJson(QJsonDocument::JsonFormat::Compact));
#endif
//    char buffTemp[sizeof(yuhu)];
//    char origin[sizeof(data_send)];

    kri.no = 12;
    //kri.c  = 2.5;
   // kr.angka = 56.09224;

    memcpy(kri.c, &data_send, sizeof(data_send));
//    qDebug()<<buffTemp << "  memcopy";

//    QByteArray baba = QByteArray::fromRawData(origin,sizeof(data_send));
   // QByteArray baTemp = QByteArray::fromRawData(buffTemp,sizeof(yuhu));
//    qDebug()<<baba.size();
//    QByteArray head = "kanal1";
//    baba.prepend(head);

    QByteArray datagram = QByteArray(static_cast<char*>((void*)&kri), sizeof(kri));

    qDebug()<<datagram.size();
    qDebug()<<datagram << "   data";
    qDebug()<<kri.no << " <-  balik lagi ke struct";
   sendDataClient1(datagram);
}

void data::datamanagement2()
{
#if 0
    QJsonObject isikanal;
    //kanal_p1
    QJsonObject kanalip2;
    QJsonObject ip2;

    QJsonArray buff1;
    QJsonArray buff2;
    QJsonArray buff3;
    QJsonArray buff4;
    QJsonValue spsip2_;


    for(int i=0; i<PAKET_10; i++)
    {
        buff1.push_back(data10paket_5[i]);
        buff2.push_back(data10paket_6[i]);
        buff3.push_back(data10paket_7[i]);
        buff4.push_back(data10paket_8[i]);
    }

    kanalip2.insert("kanal1",buff1);
    kanalip2.insert("kanal2",buff2);
    kanalip2.insert("kanal3",buff3);
    kanalip2.insert("kanal4",buff4);
    kanalip2.insert("spsip1",spsX);

    //memasukkan seluruh data kanal ke doc json
    //jika sudah melakukan parsing dan membaca dan ditampilkan

    ip2.insert("ip1",kanalip2);
    QJsonDocument paket2Bro(ip2);
    QString sout2 = QString::fromUtf8(paket2Bro.toJson(QJsonDocument::JsonFormat::Compact));
    sendDataClient2(sout2);
#endif
}

void data::start_database()
{

    qDebug("%s() ====================================> %llu",__FUNCTION__,QDateTime::currentSecsSinceEpoch());
    qDebug() << "sizeof(float) = " << sizeof(float);
    qDebug() << "jumlah channel "<< cnt_ch1 << "spektrum point "<< spektrum_points;

    for(int i =0; i<JUM_PLOT; i++)
    {
        qDebug()<<"cnt_ch "<<i+1<<" = "<<cnt_ch[i];
        if(cnt_ch[i] == 0 )
        {
            qDebug()<<"Not safe to save data kanal "<<i+1;
            threadku->safe_to_save_ch[i] = 0;
            continue;
        }
        else
        {
            threadku->safe_to_save_ch[i] = 1;
        }

        memcpy(&data_get[i][0], &data_save[i][cnt_ch[i]-(spektrum_points)], spektrum_points * (sizeof(float)));

        QByteArray array0((char *) &data_get[i][0], spektrum_points * sizeof(float));
       // QByteArray array0((char *)&data_save[i][0],spektrum_points * sizeof(float));
        threadku->bb1[i] = array0;
        threadku->ref_rpm = 6000;
        threadku->num = spektrum_points;
        threadku->fmax = 1000;

        threadku->start();
        qDebug() << "akan save 1";
        /* clear memory and variable */

        array0.clear();
    }
        for(int i =0; i<JUM_PLOT; i++)
        {
            cnt_ch[i] =0;
        }
}

void data::refresh_plot()
{
    req_UDP();
    tim_count++;
}

void data::onNewConnection()
{
    C_NewCon = m_pWebSocketServer1->nextPendingConnection();
    connect(C_NewCon, &QWebSocket::binaryMessageReceived, this, &data::sendDataClient1);
  //  connect(C_NewCon, &QWebSocket::textMessageReceived, this, &data::processMessage);
    connect(C_NewCon, &QWebSocket::disconnected, this, &data::socketDisconnected);
    CG_NewClient << C_NewCon; //grup conneksi
}

void data::processMessage(QByteArray message)
{
   // QWebSocket *C_NewReq = qobject_cast<QWebSocket *>(sender());
        qDebug()<<message;
        QByteArray ba1;
        QByteArray ba2;
        QByteArray bukanAnggota;
        QString wave1 ="wave1";
        QString unsub_wave1 ="unsub_wave1";
        QString bukan = "bukan register kita";
        bukanAnggota += bukan;
        ba1 += wave1;
        ba2 += unsub_wave1;
        //QString unsub_wave2 ="unsub_wave2";
       // QString bukanAnggota ="Maaf req anda tidak terdaftar";
//        if((C_NewReq)&&(message==ba1))
//            {
//                   Subcribe_wave1.removeAll(C_NewReq);
//                   Subcribe_wave1 << C_NewReq;
//                   qDebug()<<"req wave 1 dari:"<<C_NewReq->peerAddress().toString();
//            }

//       if((C_NewReq)&&(message==ba2))
//           {
//               Subcribe_wave1.removeAll(C_NewReq);
//               C_NewReq->deleteLater();
//               QHostAddress join=C_NewReq->peerAddress();
//               QString loststr=join.toString();
//               qDebug()<<"client unsub wave1" << loststr;
//           }
//#if 0
//       else if((Subcribe_wave1.indexOf(C_NewCon))&&(message==wave2))
//        {
//           Subcribe_wave2 << C_NewCon;////paket10
//           qDebug()<<"req wave 2 dari:"<<C_NewCon->peerAddress().toString();
//        }
//       else if(message==unsub_wave2)
//       {
//           Subcribe_wave2.removeAll(C_NewCon);
//           C_NewCon->deleteLater();
//           QHostAddress join=C_NewCon->peerAddress();
//           QString loststr=join.toString();
//           qDebug()<<"client unsub wave1" << loststr;
//       }
//#endif
//       else
//       {
//           message=bukanAnggota;
//         //  C_NewReq->sendTextMessage(message);
//           C_NewReq->sendBinaryMessage(message);
//       }

}

void data::sendDataClient1(QByteArray isipesan)
{
    Q_FOREACH (pClientkirim, CG_NewClient)//paket10
    {
        QHostAddress join=pClientkirim->peerAddress();
        QString joinstr=join.toString();
        qDebug() << "kirim paket 1----ke : "<<joinstr;
        pClientkirim->sendBinaryMessage(isipesan);
    }
}

void data::sendDataClient2(QString isipesan2)
{
#if 1
    Q_FOREACH (pClientkirim, Subcribe_wave2)//paket10
    {
        QHostAddress join=pClientkirim->peerAddress();
        QString joinstr=join.toString();
        qDebug() << "kirim paket 2----ke : "<<joinstr;
        pClientkirim->sendTextMessage(isipesan2);
    }
#endif
}


void data::socketDisconnected()
{
    pClient1 = qobject_cast<QWebSocket *>(sender());
    if (pClient1)
    {
        //pClient1->peerAddress();
        Subcribe_wave1.removeAll(pClient1);//paket10
        Subcribe_wave2.removeAll(pClient1);//paket10
        pClient1->deleteLater();
        C_NewCon->deleteLater();
        pClientkirim->deleteLater();
        QHostAddress join=pClient1->peerAddress();
        QString loststr=join.toString();
        qDebug()<<"client loss" << loststr;
    }
    pClient1->deleteLater();
    C_NewCon->deleteLater();
}
