/*
This program is free software. Released under terms of GNU LGPL 2.1
Author	: Marcin Ka³at http://mkalat.pl
Version : 0.0.7



*/

#include "WishesSenderPlugin.h"

#include <QTextCodec>
#include <QFile>
#include <QMessageBox>
#include <windows.h>
#include <process.h>
#include <tchar.h>
#include <core/util.h>
#include <gui/TlenContextMenu.h>
#include <plugin/TlenAccountConnection.h>
#include <plugin/TlenAccountManager.h>
#include <plugin/TlenPluginManager.h>
#include <plugin/TlenProtocol.h>
#include <roster/TlenRoster.h>
#include <debug/TlenDebug.h>
#include <data/TlenBuddy.h>
#include <gui/TlenChatManager.h>
#include <gui/TlenChatWindow.h>
#include <settings/TlenSettingsManager.h>
#include <gui/TlenNotificationMethod.h>




QStringList logs_urodz;
TlenBuddyGroup bday14;
TlenBuddyGroup bday7;
TlenBuddyGroup bday;
//void TH_bday_checker(void*);
QFile errlog;



WishesSenderPlugin::WishesSenderPlugin()
{
	QTextCodec *codec = QTextCodec::codecForName("Windows-1250");
	QStringList null_list;
	null_list.append("");
	null_list.append(""); 
	addPluginSettingsField(TlenField::checkBox(codec->toUnicode("Twórz specjaln¹ grupê dla kontaktów maj¹cych urodziny"),"chkb_grupa",false,"",null_list));
	addPluginSettingsField(TlenField::checkBox(codec->toUnicode("Oznaczaj na liœcie kontaktów kontakty maj¹ce urodziny"),"chkb_bday",false,"",null_list));
	addPluginSettingsField(TlenField::checkBox(codec->toUnicode("Wysy³aæ automatycznie ¿yczenia"),"auto_send_wish",false,"",null_list)); 
	addPluginSettingsField(TlenField::textEdit(codec->toUnicode("Wpisz ¿yczenia jakie wys³aæ"),"msg_wish","Wszystkiego najlepszego z okazji urodzin","")); 
	addPluginSettingsField(TlenField::spinBox(codec->toUnicode("Minimalny wiek solenizanta dla powiadomieñ"),"min_age",100,0,7,"",""));
	startTimer(10000);
}

bool WishesSenderPlugin::load()
{	
	errlog.setFileName("wsp_log.txt");
	errlog.open(QFile::WriteOnly | QFile::Text);
	errlog.write("Wishes Sender Plugin 0.0.6 - LOGGING STARTED \n");

	TlenPluginManager *pm = TlenPluginManager::getInstance();

	errlog.write("Tlen Plugin Manager \n");

	foreach (TlenProtocol *proto, pm->getRegisteredProtocols()) {		
		slotConnect(proto->getPluginId(),ACC_LOGGED_IN,TLEN_SLOT(WishesSenderPlugin::account_logged_in));
		
		
	}
	slotConnect(TLEN_PLUGIN_CORE,APP_QUITTING, TLEN_SLOT(WishesSenderPlugin::tlen_exit));
	bool result = slotConnect(TLEN_PLUGIN_CORE, LOADED_PROTOCOL, TLEN_SLOT(WishesSenderPlugin::protocolLoaded));
	
	errlog.write("Signals connected \n");
	
	ClearBDB();
	QTextCodec *codec = QTextCodec::codecForName("Windows-1250");
	TlenField tf_grupa;
	tf_grupa = getPluginPref("chkb_grupa");

	errlog.write("Checking if group option is active \n");

	if (tf_grupa.isChecked() == true)
	{
		errlog.write("Grouping option is active - creating contacts groups \n");
		TlenRoster* t_rost = TlenRoster::getInstance();
		bday14 = t_rost->addGroup(codec->toUnicode("Kontakty maj¹ce urodziny za 14 dni"));
		bday7 = t_rost->addGroup(codec->toUnicode("Kontakty maj¹ce urodziny za 7 dni"));
		bday = t_rost->addGroup(codec->toUnicode("Kontakty maj¹ce urodziny dziœ"));
	}


	errlog.write("plugin loaded successfully\n");

	return result;
}

void WishesSenderPlugin::unload()
{
	errlog.write("Unloading plugin started....\n");
	Clear_grupy();
	
	
	Clear_disp();
	ClearBDB();
	errlog.write("Plugin ready for unload - LOGGING STOPPED. \n");
	errlog.close();
	
}

QString WishesSenderPlugin::name() const
{
	return QString("WishesSenderPlugin");
}

QString WishesSenderPlugin::friendlyName() const
{
	return QObject::tr("Wishes Sender Plugin");	
}

QString WishesSenderPlugin::icon(int size) const
{
	return QString("");
}

QString WishesSenderPlugin::author() const
{
	QTextCodec *codec = QTextCodec::codecForName("Windows-1250");
	return QString(codec->toUnicode("Marcin Ka³at"));
}

QString WishesSenderPlugin::description() const
{
	QTextCodec *codec = QTextCodec::codecForName("Windows-1250");

	
	return QString(codec->toUnicode("Wtyczka przypominaj¹ca i wysy³aj¹ca ¿yczenia do osób z listy kontaktów"));
	
}

int WishesSenderPlugin::version() const
{
	return TLEN_PLUGIN_VERSION(0, 0, 7);
}

QString WishesSenderPlugin::web() const
{
	return QString("http://www.mkalat.waw.pl");
}

QString WishesSenderPlugin::email() const
{
	return QString("support@mkalat.waw.pl");
}
QString WishesSenderPlugin::getLicenseName() const
{
	return QString("GNU LGPL 2.1");
}






TLEN_DEFINE_SLOT(WishesSenderPlugin, protocolLoaded)
{	
	errlog.write("Singal handler - proto loaded is beeing processed ... \n");
	QString protocol = args[1].toPlugin()->getPluginId();
	slotConnect(protocol,ACC_LOGGED_IN,TLEN_SLOT(WishesSenderPlugin::account_logged_in));
	errlog.write("Signal handler - proto loaded processed \n");
		
}

TLEN_DEFINE_SLOT(WishesSenderPlugin, account_logged_in)
{
	TlenAccountConnection* acc = args[0].toAccount(); 

	
}


void WishesSenderPlugin::timerEvent(QTimerEvent *event)  
{
	errlog.write("Timer Event()....\n");
	TlenRoster* t_rost = TlenRoster::getInstance(); 
	TlenAccountManager* tam = TlenAccountManager::getInstance(); 
	
	errlog.write("Got Tlen Roster and TAM instance.... \n");

	if (dzis_data != QDate::currentDate().toString("yyyy-mm-dd"))  
	{
		dzis_data = QDate::currentDate().toString("yyyy-mm-dd");
		Clear_grupy();		
		Clear_disp();
		ClearBDB();
		errlog.write("Detected changed date - cleared plugin db...\n");
	}

	
	foreach(TlenAccountConnection* acc, tam->getLoggedInAccounts())
	{
		errlog.write("TAM - logged in account loop... \n");
		QTextCodec *codec = QTextCodec::codecForName("Windows-1250");
		QString hello_comm;
		hello_comm = codec->toUnicode(" ma dziœ urodziny ! ");
		QString comm, comm2, comm3, comm4;
		
	
		int day_b, mnth_b, year_b, day_c, mnth_c, year_c;
		
		foreach(const TlenBuddyContact &bud_c, t_rost->getBuddiesForAccount(acc))
		{
			errlog.write("TBC in TR loop \n");
			if (bud_c.isValid())
			{
				errlog.write("Still inside TBC loop - Buddy is valid...\n");
				foreach(const TlenField &tf, bud_c.getExtraInfo())
				{
					errlog.write("TF loop...\n");
					if (tf.getId() == "birthday")
					{
						errlog.write("detected birthday field\n");
						QDate b_data = tf.getDate();
						if (!b_data.isNull()) 
						{
							errlog.write("bithday field is not null - further processing will occur \n");
							/*QMessageBox qmsg1(QMessageBox::Information,"Buddy Info",
								bud_c.getDisplay() + " - " + b_data.toString("yyyy-MM-dd") + " - " + QDate::currentDate().toString("yyyy-MM-dd"),
								QMessageBox::Ok,NULL,Qt::MSWindowsFixedSizeDialogHint);
							qmsg1.exec(); */
							day_b = b_data.toString("dd").toInt(0,10);
							mnth_b = b_data.toString("MM").toInt(0,10); 
							year_b = b_data.toString("yyyy").toInt(0,10);
							day_c = QDate::currentDate().toString("dd").toInt(0,10);
							mnth_c = QDate::currentDate().toString("MM").toInt(0,10);
							year_c = QDate::currentDate().toString("yyyy").toInt(0,10);
							QDate data_ur(year_c,mnth_b,day_b);
							QDate data_cur(year_c,mnth_c,day_c);
							
							errlog.write("got cur date \n");

							TlenField tf_bdy_name;
							TlenField tf_bdy_grupa;
							TlenField tf_min_age;
							tf_bdy_grupa = getPluginPref("chkb_grupa");
							tf_bdy_name = getPluginPref("chkb_bday");
							tf_min_age = getPluginPref("min_age");

							errlog.write("got TlenField \n");

							QHash<QString,TlenArg> null_hash;
							null_hash.insert("",0);
							if (!((year_c - year_b) < tf_min_age.getIntValue()))
							{
								errlog.write("Contact is within selcted age \n");
								if (CheckEX(bud_c.getDisplay()))
								{
									errlog.write("Contact does not exist in DB \n");
									if (data_cur.daysTo(data_ur) == 14)
									{
										errlog.write("Contact has bithday for 14 days \n");
										comm3 = bud_c.getDisplay() + " ma za 14 dni urodziny !";
										postNotification(INFO_NOTIFY,"Wishes Sender Plugin",comm3,icon(0),"",TlenPluginAction());
										errlog.write("Notification sent that contact has bithday for 14 days \n");
										
										if (tf_bdy_name.isChecked() == true)
										{	
											errlog.write("Marking on roster option is selected..\n");
											QString qstr_bdy_name;
											TlenBuddy bud_chng = bud_c;
											qstr_bdy_name = bud_chng.getDisplay();
											qstr_bdy_name = qstr_bdy_name + " * urodziny za 14 dni";
											t_rost->renameBuddy(bud_chng,qstr_bdy_name);
											errlog.write("Contact marked on roster that it has birthdays for 14 days \n");


										}
										if (tf_bdy_grupa.isChecked() == true)
										{
											errlog.write("Adding Contacts that has bithday for 14 day to special group option is selected \n");
											if (bday14.isValid() == true)
											{
												errlog.write("buddy is valid :) \n");
												QString bud_name;
												bud_name = bud_c.getDisplay();
												bud_name.remove(codec->toUnicode(" * urodziny za 14 dni"),Qt::CaseInsensitive);
												t_rost->addCustom(bud_name,bday14.getDisplay(),"",getPluginId(),null_hash,"","","");								
												errlog.write("added contact that has birthday for 14 to special group on roster \n");

											}
										}
										log_bd.append(bud_c.getDisplay());
										errlog.write("Contact added to DB - user will not be notified again, returning \n");
										return;

									}
									if (data_cur.daysTo(data_ur) == 7)
									{
										errlog.write("Contact has birthday for 7 days \n");
										comm4 = bud_c.getDisplay() + " ma za 7 dni urodziny !";
										postNotification(INFO_NOTIFY,"Wishes Sender Plugin",comm4,icon(0),"",TlenPluginAction());
										errlog.write("Notification sent that contact has birthday for 7 days \n");
										
										if (tf_bdy_name.isChecked() == true)
										{	
											errlog.write("Marking on roster that contact has birthday for 7 days option is selected \n");
											QString qstr_bdy_name;
											TlenBuddy bud_chng = bud_c;
											qstr_bdy_name = bud_chng.getDisplay();
											qstr_bdy_name = qstr_bdy_name + " * urodziny za 7 dni";
											t_rost->renameBuddy(bud_chng,qstr_bdy_name);
											errlog.write("Contact has been marked that it has birthday for 7 days \n");


										}
										if (tf_bdy_grupa.isChecked() == true)
										{
											errlog.write("Adding Contacts that have birthday for 7 days to special group option selected\n");
											if (bday7.isValid() == true)
											{
												errlog.write("Buddy is valid \n");
												QString bud_name;
												bud_name = bud_c.getDisplay();
												bud_name.remove(codec->toUnicode(" * urodziny za 7 dni"),Qt::CaseInsensitive);
												t_rost->addCustom(bud_name,bday7.getDisplay(),"",getPluginId(),null_hash,"","","");								

												errlog.write("Contact Added to group grouping contacts that have birtday for 7 days \n");
											}
										}
										log_bd.append(bud_c.getDisplay());
										errlog.write("Contact added to DB, user will not be notified again today - returning function \n");
										return;
									}
									
									if (QDate::currentDate().toString("MM-dd") == b_data.toString("MM-dd"))
									{
										/*QMessageBox qmsg3(QMessageBox::Information,"Buddy Info",
										bud_c.getDisplay() + " - " + b_data.toString("MM-dd") + " - " + QDate::currentDate().toString("MM-dd"),
										QMessageBox::Ok,NULL,Qt::MSWindowsFixedSizeDialogHint);
										qmsg3.exec();*/ 
										errlog.write("Contact has birthday today \n");
										comm = bud_c.getDisplay() + hello_comm;
										
										postNotification(INFO_NOTIFY,"Wishes Sender Plugin",comm,icon(0),"",TlenPluginAction());
										
										errlog.write("Notification that Contact have birthday today sent \n");

										TlenField tf_wishes;
										tf_wishes = getPluginPref("msg_wish");
										TlenField tf_auto_send;
										tf_auto_send = getPluginPref("auto_send_wish");

										errlog.write("Tlen Filed read \n");
										if (tf_auto_send.isChecked() == true)
										{
											errlog.write("Auto Sending Wishes option selected \n");
											TlenProtocol *proto = acc->getProtocol();
											proto->sendMessage(acc,bud_c.getId(),tf_wishes.getText(),"Tlen","");

											errlog.write("Wishes sent....\n");

											comm2 = codec->toUnicode("Wys³ano ¿yczenia do ") + bud_c.getDisplay();
											postNotification(INFO_NOTIFY,"Wishes Sender Plugin",comm2,icon(0),"",TlenPluginAction()); 

											errlog.write("Notufication about sent wishes sent \n");
										}
										if (tf_bdy_name.isChecked() == true)
										{	
											errlog.write("Marking contact that have birthday today option selected \n");
											QString qstr_bdy_name;
											TlenBuddy bud_chng = bud_c;
											qstr_bdy_name = bud_chng.getDisplay();
											qstr_bdy_name = qstr_bdy_name + codec->toUnicode(" * DZIŒ MA URODZINY");
											t_rost->renameBuddy(bud_chng,qstr_bdy_name);
											errlog.write("Contact that have birthday today marked on roster \n");


										}
										if (tf_bdy_grupa.isChecked() == true)
										{
											errlog.write("Adding Contact that have birthday today to special group option selected \n");
											if (bday.isValid() == true)
											{
												errlog.write("Buddy is valid \n");
												QString bud_name;
												bud_name = bud_c.getDisplay();
												bud_name.remove(codec->toUnicode(" * DZIŒ MA URODZINY"),Qt::CaseInsensitive);
												t_rost->addCustom(bud_name,bday.getDisplay(),"",getPluginId(),null_hash,"","","");								
												errlog.write("Contact that have birthday today added to special group \n");
												

											}
										}
										log_bd.append(bud_c.getDisplay());
										errlog.write("Contact added to DB, user will not be notified again today\n");
										return;

									}
								}
							}

						}
						else
						{
							/*QMessageBox qmsg1(QMessageBox::Information,"Buddy Info",
								bud_c.getDisplay() + " - " + b_data.toString(Qt::TextDate) + "DATA = NULL",
								QMessageBox::Ok,NULL,Qt::MSWindowsFixedSizeDialogHint);
							qmsg1.exec(); */

						}

					}



				}



			}



		}

	}

	
				

}

bool WishesSenderPlugin::CheckEX(QString log) 
{
	errlog.write("Checking if contact is in db ....\n");
 for (int x = 0; x < log_bd.count(); x++)
 {
	if (log_bd[x] == log)
	{
		errlog.write("Contact is in DB - theres no need to notify user, already notified \n");
		return false;
	}
 }
 errlog.write("Theres no such Contact in DB, user will be notified according to selected options \n");
return true;

}
void WishesSenderPlugin::ClearBDB() 
{
	errlog.write("ClearBDB()\n");
 if (log_bd.empty() == false) 
 {
	 errlog.write("DB not empty - clearing ...\n");
	 log_bd.clear();  
 }

}

void WishesSenderPlugin::Clear_disp()
{
	errlog.write("Clearing all changes made by plugin.... \n");
	TlenRoster* t_rost = TlenRoster::getInstance();
	TlenAccountManager* tam = TlenAccountManager::getInstance();

	errlog.write("Tlen Roster Tlen Account Manager..\n");

	QTextCodec *codec = QTextCodec::codecForName("Windows-1250");
	QString bud_name;
	foreach(TlenAccountConnection* acc, tam->getLoggedInAccounts())
	{
		errlog.write("TAM loop.\n");
		foreach(const TlenBuddy &bud, t_rost->getBuddiesForAccount(acc))
		{
			errlog.write("TB loop \n");
			if (bud.getDisplay().contains(" * urodziny za 14 dni",Qt::CaseInsensitive))
			{
				errlog.write("Found bdy14 modification .. clearing ...\n");
				bud_name = bud.getDisplay();
				bud_name.remove(" * urodziny za 14 dni",Qt::CaseInsensitive);
				t_rost->renameBuddy(bud,bud_name);
				errlog.write("Cleared.\n");

			}
			else if (bud.getDisplay().contains(" * urodziny za 7 dni",Qt::CaseInsensitive))
			{
				errlog.write("Found bdy7 modofication.. clearing ... \n");
				bud_name = bud.getDisplay();
				bud_name.remove(" * urodziny za 7 dni",Qt::CaseInsensitive);
				t_rost->renameBuddy(bud,bud_name);
				errlog.write("Cleared.\n");

			}
			else if (bud.getDisplay().contains(codec->toUnicode(" * DZIŒ MA URODZINY"),Qt::CaseInsensitive))
			{
				errlog.write("Found bdy0 modification.. clearing ... \n");
				bud_name = bud.getDisplay();
				bud_name.remove(codec->toUnicode(" * DZIŒ MA URODZINY"),Qt::CaseInsensitive);
				t_rost->renameBuddy(bud,bud_name);
				errlog.write("Cleared.\n");

			}
		}
	}


}

void WishesSenderPlugin::Clear_grupy()
{
	errlog.write("Clear_grupy()\n");
	TlenRoster* t_rost = TlenRoster::getInstance();
	foreach (TlenBuddyCustom bud_cust, t_rost->getCustomBuddiesByOwner(getPluginId()))
	{
		errlog.write("TBC loop by owner \n");
		t_rost->removeCustom(bud_cust);
		

	}

}
TlenPluginAction WishesSenderPlugin::customBuddyActivated()
{
	return TlenPluginAction(this,TLEN_ACTION(WishesSenderPlugin::bdy_activated),QList<TlenArg>());
}
TLEN_DEFINE_ACTION(WishesSenderPlugin,bdy_activated)
{


}
TLEN_DEFINE_SLOT(WishesSenderPlugin,tlen_exit)
{
	errlog.write("Tlen closing ... preparing for plugin unload..\n");
	Clear_grupy();
	Clear_disp();
	ClearBDB();
	errlog.write("App Quiting - LOGGING STOPPED.\n");
	errlog.close();


}




TLEN_INIT_PLUGIN(WishesSenderPlugin)