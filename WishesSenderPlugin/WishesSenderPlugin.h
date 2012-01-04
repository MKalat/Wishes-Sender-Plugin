#ifndef WISHES_SENDER_PLUGIN_H
#define WISHES_SENDER_PLUGIN_H


#include <QObject>
#include <QTimer>
/*
This program is free software. Released under terms of GNU LGPL 2.1
Author	: Marcin Ka³at http://mkalat.pl
Version : 0.0.7



*/

#include <plugin/TlenPlugin.h>



class WishesSenderPlugin : public QObject, public TlenPluginRosterHandler, public TlenPlugin 
{
	Q_OBJECT
public:
	
	WishesSenderPlugin();

	bool load();
	void unload();

	QString name() const;
	QString friendlyName() const;
	QString icon(int size=0) const;
	QString author() const;
	QString description() const;
	int version() const;
	QString web() const;
	QString email() const;
	QString getLicenseName() const;

	QString dzis_data;
	QList<QString> log_bd;
	bool CheckEX(QString log);
	void ClearBDB();
	void Clear_disp();
	void Clear_grupy();
	
	TLEN_DECLARE_SLOT(protocolLoaded);
	TLEN_DECLARE_SLOT(account_logged_in);
	TLEN_DECLARE_SLOT(tlen_exit);
	TLEN_DECLARE_ACTION(bdy_activated);

protected :
	void timerEvent(QTimerEvent *event);
	virtual TlenPluginAction customBuddyActivated();


};



#endif