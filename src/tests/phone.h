#include <QObject>

class SipClient;

class PhoneTester : public QObject
{
    Q_OBJECT

public:
    PhoneTester(QObject *parent = 0);
    void start();
    void stop();

signals:
    void finished();

private slots:
    void connected();

private:
    SipClient *m_client;
    QString m_phoneNumber;
};

