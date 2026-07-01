#pragma once
#include <QObject>

class ThemeManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString primaryColor READ primaryColor CONSTANT)
    Q_PROPERTY(QString accentColor READ accentColor CONSTANT)
public:
    explicit ThemeManager(QObject *parent = nullptr);
    QString primaryColor() const { return "#1A73E8"; }
    QString accentColor() const { return "#FF6D00"; }
};
