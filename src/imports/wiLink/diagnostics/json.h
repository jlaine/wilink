#include <QJsonArray>

template <class T>
QJsonArray listToJson(const QList<T> &items)
{
    QJsonArray list;
    foreach (const T &item, items)
        list << item.toJson();
    return list;
}
