#include <QObject>
#include <QStringList>

class SipCall;
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
    void received(SipCall *call);

private:
    SipClient *m_client;
    QStringList m_phoneQueue;
};

