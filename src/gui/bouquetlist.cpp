/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Copyright (C) 2009,2011,2013,2015-2017 Stefan Seyfried
	Copyright (C) 2017 Sven Hoefer

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>

#include <gui/bouquetlist.h>

#include <gui/color.h>
#include <gui/eventlist.h>
#include <gui/infoviewer.h>

#include <gui/components/cc.h>
#include <gui/widget/menue.h>
#include <gui/widget/buttons.h>
#include <gui/widget/icons.h>
#include <gui/widget/msgbox.h>
#include <gui/infoclock.h>
#include <driver/display.h>
#include <driver/fontrenderer.h>
#include <driver/screen_max.h>
#include <driver/rcinput.h>
#include <driver/fade.h>
#include <driver/display.h>
#include <driver/scanepg.h>

#include <daemonc/remotecontrol.h>
#include <system/settings.h>

#include <global.h>
#include <neutrino.h>
#include <mymenu.h>
#include <zapit/getservices.h>

#ifdef ENABLE_LCD4LINUX
#include "driver/lcd4l.h"
extern CLCD4l *LCD4l;
#endif

extern CBouquetManager *g_bouquetManager;

CBouquetList::CBouquetList(const char * const Name)
{
	frameBuffer = CFrameBuffer::getInstance();
	selected    = 0;
	liststart   = 0;
	favonly     = false;
	save_bouquets = false;
	if(Name == NULL)
		name = g_Locale->getText(LOCALE_BOUQUETLIST_HEAD);
	else
		name = Name;
}

CBouquetList::~CBouquetList()
{
	for (std::vector<CBouquet *>::iterator it = Bouquets.begin(); it != Bouquets.end(); ++it)
		delete (*it);

	Bouquets.clear();
}

CBouquet* CBouquetList::addBouquet(CZapitBouquet * zapitBouquet)
{
	int BouquetKey= Bouquets.size();//FIXME not used ?
	CBouquet* tmp = new CBouquet(BouquetKey, zapitBouquet->bName.c_str(), zapitBouquet->bLocked, !zapitBouquet->bUser);
	tmp->zapitBouquet = zapitBouquet;
	Bouquets.push_back(tmp);
	return tmp;
}

CBouquet* CBouquetList::addBouquet(const char * const pname, int BouquetKey, bool locked)
{
	if ( BouquetKey==-1 )
		BouquetKey= Bouquets.size();

	CBouquet* tmp = new CBouquet( BouquetKey, pname, locked, true);
	Bouquets.push_back(tmp);
	return(tmp);
}

void CBouquetList::deleteBouquet(CBouquet*bouquet)
{
	if (bouquet != NULL) {
		std::vector<CBouquet *>::iterator it = find(Bouquets.begin(), Bouquets.end(), bouquet);

		if (it != Bouquets.end()) {
			Bouquets.erase(it);
			delete bouquet;
		}
	}
}

t_bouquet_id CBouquetList::getActiveBouquetNumber()
{
	return (t_bouquet_id)selected;
}

#if 0
void CBouquetList::adjustToChannel( int nChannelNr)
{
	for (uint32_t i=0; i<Bouquets.size(); i++) {
		int nChannelPos = Bouquets[i]->channelList->hasChannel(nChannelNr);
		if (nChannelPos > -1) {
			selected = i;
			Bouquets[i]->channelList->setSelected(nChannelPos);
			return;
		}
	}
}
#endif
bool CBouquetList::hasChannelID(t_channel_id channel_id)
{
	for (uint32_t i = 0; i < Bouquets.size(); i++) {
		int nChannelPos = Bouquets[i]->channelList->hasChannelID(channel_id);
		if (nChannelPos > -1)
			return true;
	}
	return false;
}

bool CBouquetList::adjustToChannelID(t_channel_id channel_id)
{
//printf("CBouquetList::adjustToChannelID [%s] to %llx, selected %d size %d\n", name.c_str(), channel_id, selected, Bouquets.size());
	if(selected < Bouquets.size()) {
		int nChannelPos = Bouquets[selected]->channelList->hasChannelID(channel_id);
		if(nChannelPos > -1) {
//printf("CBouquetList::adjustToChannelID [%s] to %llx -> not needed\n", name.c_str(), channel_id);
			Bouquets[selected]->channelList->setSelected(nChannelPos);
			return true;
		}
	}
//printf("CBouquetList::adjustToChannelID [%s] to %llx\n", name.c_str(), channel_id);
	for (uint32_t i=0; i < Bouquets.size(); i++) {
		if(i == selected)
			continue;
		int nChannelPos = Bouquets[i]->channelList->hasChannelID(channel_id);
		if (nChannelPos > -1) {
			selected = i;
			Bouquets[i]->channelList->setSelected(nChannelPos);
			return true;
		}
	}
	return false;
}
/* used in channellist to switch bouquets up/down */
int CBouquetList::showChannelList( int nBouquet)
{
	if ((nBouquet < 0)|| (nBouquet >= (int) Bouquets.size()))
		nBouquet = selected;

	int nNewChannel = Bouquets[nBouquet]->channelList->exec();
	if (nNewChannel > -1) {
		selected = nBouquet;
		nNewChannel = -2;
	}
	return nNewChannel;
}
/* bShowChannelList default to false , return seems not checked anywhere */
int CBouquetList::activateBouquet( int id, bool bShowChannelList)
{
	int res = -1;

	if((id >= 0) && (id < (int) Bouquets.size()))
		selected = id;

	if (bShowChannelList) {
		res = Bouquets[selected]->channelList->exec();
		if(res > -1)
			res = -2;
	}
	return res;
}

int CBouquetList::exec( bool bShowChannelList)
{
	/* select bouquet to show */
	int res = show(bShowChannelList);
//printf("Bouquet-exec: res %d bShowChannelList %d\n", res, bShowChannelList); fflush(stdout);

	if(!bShowChannelList)
		return res;
	/* if >= 0, call activateBouquet to show channel list */
	if ( res > -1) {
		return activateBouquet(selected, bShowChannelList);
	}
	return res;
}

int CBouquetList::doMenu()
{
	int i = 0, ret = 0;
	int select = -1;
	static int old_selected = 0;
	signed int bouquet_id;
	char cnt[5];
	CZapitBouquet * tmp, * zapitBouquet;
	ZapitChannelList* channels;

	if(Bouquets.empty() || g_settings.minimode)
		return 0;

	zapitBouquet = Bouquets[selected]->zapitBouquet;
	/* zapitBouquet not NULL only on real bouquets, satellitePosition is set for providers or SAT */
	if(!zapitBouquet && Bouquets[selected]->satellitePosition == INVALID_SAT_POSITION)
		return 0;

	CMenuWidget* menu = new CMenuWidget(LOCALE_CHANNELLIST_EDIT, NEUTRINO_ICON_SETTINGS);
	menu->enableFade(false);
	menu->enableSaveScreen(true);
	CMenuSelectorTarget * selector = new CMenuSelectorTarget(&select);

	int old_epg = zapitBouquet ? zapitBouquet->bScanEpg : 0;
	int old_ci = zapitBouquet ? zapitBouquet->bUseCI : 0;
	sprintf(cnt, "%d", i);
	/* FIXME menu centered different than bouquet list ??? */
	/* provider bouquet */
	if (zapitBouquet && !zapitBouquet->bUser) {
		menu->addItem(new CMenuForwarder(LOCALE_FAVORITES_COPY, true, NULL, selector, cnt, CRCInput::RC_blue), old_selected == i ++);
		if ((!zapitBouquet->bWebtv && !zapitBouquet->bWebradio) && g_settings.epg_scan == CEpgScan::SCAN_SEL)
			menu->addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_EPG_SCAN, &zapitBouquet->bScanEpg, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
		if (!zapitBouquet->bWebtv && !zapitBouquet->bWebradio)
			menu->addItem(new CMenuOptionChooser(LOCALE_CI_USE, &zapitBouquet->bUseCI, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
		menu->exec(NULL, "");
		delete menu;
		delete selector;
		printf("CBouquetList::doMenu: %d selected\n", select);
		if (old_epg != zapitBouquet->bScanEpg) {
			save_bouquets = true;
			CNeutrinoApp::getInstance()->MarkBouquetsChanged();
			ret = -1;
		}
		if (old_ci != zapitBouquet->bUseCI) {
			channels = &zapitBouquet->tvChannels;
			for(int li = 0; li < (int) channels->size(); li++)
				(*channels)[li]->bUseCI = zapitBouquet->bUseCI;

			channels = &zapitBouquet->radioChannels;
			for(int li = 0; li < (int) channels->size(); li++)
				(*channels)[li]->bUseCI = zapitBouquet->bUseCI;

			CServiceManager::getInstance()->SetCIFilter();
			save_bouquets = true;
			CNeutrinoApp::getInstance()->MarkBouquetsChanged();
			ret = -1;
		}

		if(select >= 0) {
			bool added = false;
			old_selected = select;
			switch(select) {
				case 0: // copy to favorites
					hide();
					bouquet_id = g_bouquetManager->existsUBouquet(Bouquets[selected]->channelList->getName());
					if(bouquet_id < 0) {
						tmp = g_bouquetManager->addBouquet(Bouquets[selected]->channelList->getName(), true);
						bouquet_id = g_bouquetManager->existsUBouquet(Bouquets[selected]->channelList->getName());
					} else
						tmp = g_bouquetManager->Bouquets[bouquet_id];

					if(bouquet_id < 0)
						return 0;

					channels = &zapitBouquet->tvChannels;
					for(int li = 0; li < (int) channels->size(); li++) {
						if (!g_bouquetManager->existsChannelInBouquet(bouquet_id, ((*channels)[li])->getChannelID())) {
							added = true;
							tmp->addService((*channels)[li]);
						}
					}
					channels = &zapitBouquet->radioChannels;
					for(int li = 0; li < (int) channels->size(); li++) {
						if (!g_bouquetManager->existsChannelInBouquet(bouquet_id, ((*channels)[li])->getChannelID())) {
							added = true;
							tmp->addService((*channels)[li]);
						}
					}
					if (added) {
						CNeutrinoApp::getInstance()->MarkFavoritesChanged();
						CNeutrinoApp::getInstance()->MarkChannelsInit();
						return 1;
					}
					break;
				default:
					break;
			}
		}
	} else {
		/* user or satellite bouquet */
		menu->addItem(new CMenuForwarder(LOCALE_BOUQUETEDITOR_DELETE, true, NULL, selector, cnt, CRCInput::RC_red), old_selected == i ++);
		if (zapitBouquet && (!zapitBouquet->bWebtv && !zapitBouquet->bWebradio) && (g_settings.epg_scan == CEpgScan::SCAN_SEL))
			menu->addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_EPG_SCAN, &zapitBouquet->bScanEpg, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
		if (zapitBouquet && (!zapitBouquet->bWebtv && !zapitBouquet->bWebradio))
			menu->addItem(new CMenuOptionChooser(LOCALE_CI_USE, &zapitBouquet->bUseCI, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

		menu->exec(NULL, "");
		delete menu;
		delete selector;
		if (zapitBouquet && (old_epg != zapitBouquet->bScanEpg)) {
			save_bouquets = true;
			CNeutrinoApp::getInstance()->MarkFavoritesChanged();
			ret = -1;
		}
		if (zapitBouquet && (old_ci != zapitBouquet->bUseCI)) {
			channels = &zapitBouquet->tvChannels;
			for(int li = 0; li < (int) channels->size(); li++)
				(*channels)[li]->bUseCI = zapitBouquet->bUseCI;

			channels = &zapitBouquet->radioChannels;
			for(int li = 0; li < (int) channels->size(); li++)
				(*channels)[li]->bUseCI = zapitBouquet->bUseCI;

			CServiceManager::getInstance()->SetCIFilter();
			save_bouquets = true;
			CNeutrinoApp::getInstance()->MarkFavoritesChanged();
			ret = -1;
		}

		printf("CBouquetList::doMenu: %d selected\n", select);
		if(select >= 0) {
			old_selected = select;

			int result = ShowMsg ( LOCALE_BOUQUETEDITOR_DELETE, Bouquets[selected]->channelList->getName(), CMsgBox::mbrNo, CMsgBox::mbYes | CMsgBox::mbNo );
			if(result != CMsgBox::mbrYes)
				return 0;

			if (zapitBouquet) {
				bouquet_id = g_bouquetManager->existsUBouquet(Bouquets[selected]->channelList->getName());
				if(bouquet_id >= 0) {
					g_bouquetManager->deleteBouquet(bouquet_id);
					CNeutrinoApp::getInstance()->MarkFavoritesChanged();
					CNeutrinoApp::getInstance()->MarkChannelsInit();
					return 1;
				}
			} else {
				CServiceManager::getInstance()->RemovePosition(Bouquets[selected]->satellitePosition);
				g_bouquetManager->loadBouquets();
				g_bouquetManager->deletePosition(Bouquets[selected]->satellitePosition);
				CNeutrinoApp::getInstance()->MarkChannelsChanged();
				CNeutrinoApp::getInstance()->MarkBouquetsChanged();
				CNeutrinoApp::getInstance()->MarkChannelsInit();
				return 1;
			}
		}
	}
	return ret;
}

const struct button_label CBouquetListButtons[4] =
{
	{ NEUTRINO_ICON_BUTTON_RED, LOCALE_CHANNELLIST_FAVS},
	{ NEUTRINO_ICON_BUTTON_GREEN, LOCALE_CHANNELLIST_PROVS},
	{ NEUTRINO_ICON_BUTTON_YELLOW, LOCALE_CHANNELLIST_SATS},
	{ NEUTRINO_ICON_BUTTON_BLUE, LOCALE_CHANNELLIST_HEAD}
};

void CBouquetList::updateSelection(int newpos)
{
	if (newpos < 0) /* to avoid all callers having to check */
		return;
	if((int) selected != newpos) {
		int prev_selected = selected;
		unsigned int oldliststart = liststart;

		selected = newpos;
		liststart = (selected/listmaxshow)*listmaxshow;
		if (oldliststart != liststart)
			paint();
		else {
			paintItem(prev_selected - liststart);
			paintItem(selected - liststart);
		}
	}
}

/* bShowChannelList default to true, returns new bouquet or -1/-2 */
int CBouquetList::show(bool bShowChannelList)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;
	int res = CHANLIST_CANCEL;
	int icol_w, icol_h;
	int w_max_text = 0;
	int w_max_icon = 0;
	int h_max_icon = 0;
	favonly = !bShowChannelList;

	for (unsigned int count = 0; count < sizeof(CBouquetListButtons)/sizeof(CBouquetListButtons[0]); count++)
	{
		int w_text = g_Font[SNeutrinoSettings::FONT_TYPE_BUTTON_TEXT]->getRenderWidth(g_Locale->getText(CBouquetListButtons[count].locale));
		w_max_text = std::max(w_max_text, w_text);
		frameBuffer->getIconSize(CBouquetListButtons[count].button, &icol_w, &icol_h);
		w_max_icon = std::max(w_max_icon, icol_w);
		h_max_icon = std::max(h_max_icon, icol_h);
	}

	item_height = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getHeight();

	/*
	   We align width to needed footer space,
	   but this manual calculation isn't a good idea.
	   It would be better to get the needed width from
	   CComponententsFooter class.
	*/
	width  = (sizeof(CBouquetListButtons)/sizeof(CBouquetListButtons[0]))*(w_max_icon + w_max_text + 2*OFFSET_INNER_MID);
	height = 16*item_height;

	header_height = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	footer_height = header_height;
	listmaxshow = (height - header_height - footer_height)/item_height;
	height      = header_height + footer_height + listmaxshow*item_height; // recalc height

	x = getScreenStartX(width);
	y = getScreenStartY(height);

	int lmaxpos= 1;
	int i= Bouquets.size();
	while ((i= i/10)!=0)
		lmaxpos++;

	CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8, "");

	COSDFader fader(g_settings.theme.menu_Content_alpha);
	fader.StartFadeIn();

	paintHead();
	paint();
	frameBuffer->blit();

	int oldselected = selected;
	int firstselected = selected+ 1;
	int zapOnExit = false;

	unsigned int chn= 0;
	int pos= lmaxpos;

	int timeout = g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST];
	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(timeout);

	bool loop=true;
	while (loop) {
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(timeout);

		if((msg == NeutrinoMessages::EVT_TIMER) && (data == fader.GetFadeTimer())) {
			if(fader.FadeDone())
				loop = false;
		}
		else if ((msg == CRCInput::RC_timeout                             ) ||
				(msg == (neutrino_msg_t)g_settings.key_channelList_cancel) ||
				((msg == CRCInput::RC_favorites) && (CNeutrinoApp::getInstance()->GetChannelMode() == LIST_MODE_FAV)))
		{
			selected = oldselected;
			if(fader.StartFadeOut()) {
				timeoutEnd = CRCInput::calcTimeoutEnd(1);
				msg = 0;
			} else
				loop=false;
		}
		else if(msg == CRCInput::RC_red || msg == CRCInput::RC_favorites) {
			if (!favonly && CNeutrinoApp::getInstance()->GetChannelMode() != LIST_MODE_FAV) {
				CNeutrinoApp::getInstance()->SetChannelMode(LIST_MODE_FAV);
				hide();
				return CHANLIST_CHANGE_MODE;
			}
		} else if(msg == CRCInput::RC_green) {
			if (!favonly && CNeutrinoApp::getInstance()->GetChannelMode() != LIST_MODE_PROV) {
				CNeutrinoApp::getInstance()->SetChannelMode(LIST_MODE_PROV);
				hide();
				return CHANLIST_CHANGE_MODE;
			}
		} else if(msg == CRCInput::RC_yellow || msg == CRCInput::RC_sat) {
			if(!favonly && bShowChannelList && CNeutrinoApp::getInstance()->GetChannelMode() != LIST_MODE_SAT) {
				CNeutrinoApp::getInstance()->SetChannelMode(LIST_MODE_SAT);
				hide();
				return CHANLIST_CHANGE_MODE;
			}
		} else if(msg == CRCInput::RC_blue) {
			if(!favonly && bShowChannelList && CNeutrinoApp::getInstance()->GetChannelMode() != LIST_MODE_ALL) {
				CNeutrinoApp::getInstance()->SetChannelMode(LIST_MODE_ALL);
				hide();
				return CHANLIST_CHANGE_MODE;
			}
		} else if(msg == CRCInput::RC_www) {
			if(!favonly && bShowChannelList && CNeutrinoApp::getInstance()->GetChannelMode() != LIST_MODE_WEB) {
				CNeutrinoApp::getInstance()->SetChannelMode(LIST_MODE_WEB);
				hide();
				return CHANLIST_CHANGE_MODE;
			}
		}
		else if ( msg == CRCInput::RC_setup) {
			if (!favonly && !Bouquets.empty()) {
				int ret = doMenu();
				if(ret > 0) {
					res = CHANLIST_NO_RESTORE;
					loop = false;
				} else if(ret < 0) {
					paintHead();
					paint();
				}
			}
		}
		else if ( msg == (neutrino_msg_t) g_settings.key_list_start ) {
			if (!Bouquets.empty())
				updateSelection(0);
		}
		else if ( msg == (neutrino_msg_t) g_settings.key_list_end ) {
			if (!Bouquets.empty())
				updateSelection(Bouquets.size()-1);
		}
		else if (msg == CRCInput::RC_up || (int) msg == g_settings.key_pageup ||
			 msg == CRCInput::RC_down || (int) msg == g_settings.key_pagedown)
		{
			int new_selected = UpDownKey(Bouquets, msg, listmaxshow, selected);
			updateSelection(new_selected);
		}
		else if(msg == (neutrino_msg_t)g_settings.key_bouquet_up || msg == (neutrino_msg_t)g_settings.key_bouquet_down) {
			if(bShowChannelList) {
				int mode = CNeutrinoApp::getInstance()->GetChannelMode();
				mode += (msg == (neutrino_msg_t)g_settings.key_bouquet_down) ? -1 : 1;
				if(mode < 0)
					mode = LIST_MODE_LAST - 1;
				else if(mode >= LIST_MODE_LAST)
					mode = 0;
				CNeutrinoApp::getInstance()->SetChannelMode(mode);
				hide();
				return CHANLIST_CHANGE_MODE;
			}
		}
		else if ( msg == CRCInput::RC_ok ) {
			if(!Bouquets.empty() /* && (!bShowChannelList || !Bouquets[selected]->channelList->isEmpty())*/) {
				zapOnExit = true;
				loop=false;
			}
		}
		else if (CRCInput::isNumeric(msg)) {
			if (!Bouquets.empty()) {
				if (pos == lmaxpos) {
					if (msg == CRCInput::RC_0) {
						chn = firstselected;
						pos = lmaxpos;
					} else {
						chn = CRCInput::getNumericValue(msg);
						pos = 1;
					}
				} else {
					chn = chn*10 + CRCInput::getNumericValue(msg);
					pos++;
				}

				if (chn > Bouquets.size()) {
					chn = firstselected;
					pos = lmaxpos;
				}

				int new_selected = (chn - 1) % Bouquets.size(); // is % necessary (i.e. can firstselected be > Bouquets.size()) ?
				updateSelection(new_selected);
			}
		} else if (msg == NeutrinoMessages::EVT_SERVICESCHANGED || msg == NeutrinoMessages::EVT_BOUQUETSCHANGED) {
			g_RCInput->postMsg(msg, data);
			loop = false;
			res = CHANLIST_CANCEL_ALL;
		} else {
			if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all ) {
				loop = false;
				res = CHANLIST_CANCEL_ALL;
			}
		}
		frameBuffer->blit();
	}
	hide();

#ifdef ENABLE_LCD4LINUX
	LCD4l->RemoveFile("/tmp/lcd/menu");
#endif

	fader.StopFade();

	CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);

	if (save_bouquets) {
		save_bouquets = false;
#if 0
		if (CNeutrinoApp::getInstance()->GetChannelMode() == LIST_MODE_FAV)
			g_bouquetManager->saveUBouquets();
		else
			g_bouquetManager->saveBouquets();
#endif
		if (g_settings.epg_scan == CEpgScan::SCAN_SEL)
			CEpgScan::getInstance()->Start();
	}

	if (zapOnExit)
		return (selected);

	return (res);
}

void CBouquetList::hide()
{
	frameBuffer->paintBackgroundBoxRel(x, y, width + OFFSET_SHADOW, height + OFFSET_SHADOW);
	CInfoClock::getInstance()->enableInfoClock(!CInfoClock::getInstance()->isBlocked());
	frameBuffer->blit();
}

void CBouquetList::paintItem(int pos)
{
	int ypos = y + header_height + pos*item_height;
	bool iscurrent = true;
	int npos = liststart + pos;
	const char * lname = NULL;

	bool i_selected	= npos == (int) selected;
	int i_radius	= RADIUS_NONE;

	fb_pixel_t color;
	fb_pixel_t bgcolor;

	getItemColors(color, bgcolor, i_selected);

	if (i_selected)
		i_radius = RADIUS_LARGE;

	if (i_radius)
		frameBuffer->paintBoxRel(x, ypos, width - SCROLLBAR_WIDTH, item_height, COL_MENUCONTENT_PLUS_0);
	frameBuffer->paintBoxRel(x, ypos, width - SCROLLBAR_WIDTH, item_height, bgcolor, i_radius);

	if (npos < (int) Bouquets.size())
		lname = Bouquets[npos]->channelList->getName();

	if (i_selected)
	{
		if(npos < (int) Bouquets.size())
			CVFD::getInstance()->showMenuText(0, lname, -1, true);
#ifdef ENABLE_LCD4LINUX
		if(g_settings.lcd4l_support)
			LCD4l->CreateFile("/tmp/lcd/menu", lname, g_settings.lcd4l_convert);
#endif
	}
	else
	{
		if(!favonly && (npos < (int) Bouquets.size()))
			iscurrent = !Bouquets[npos]->channelList->isEmpty();

		if (!iscurrent)
		{
			//inactive colors? Is this correct?
			color = COL_MENUCONTENTINACTIVE_TEXT;
			//bgcolor = COL_MENUCONTENTINACTIVE_PLUS_0;
		}
	}

	if (npos < (int) Bouquets.size()) {
		char num[10];
		sprintf((char*) num, "%d", npos + 1);
		int iw = 0, ih = 0;
		if ((g_settings.epg_scan == CEpgScan::SCAN_SEL) &&
				Bouquets[npos]->zapitBouquet && Bouquets[npos]->zapitBouquet->bScanEpg) {
			frameBuffer->getIconSize(NEUTRINO_ICON_EPG, &iw, &ih);
			if (iw && ih) {
				int icon_x = x + width - SCROLLBAR_WIDTH - OFFSET_INNER_MID - iw;
				frameBuffer->paintIcon(NEUTRINO_ICON_EPG, icon_x, ypos, item_height);
				iw = iw + OFFSET_INNER_MID;
			}
		}
		if (Bouquets[npos]->zapitBouquet && Bouquets[npos]->zapitBouquet->bUseCI) {
			int iw2 = 0;
			frameBuffer->getIconSize(NEUTRINO_ICON_SCRAMBLED, &iw2, &ih);
			if (iw2 && ih) {
				int icon_x = x + width - SCROLLBAR_WIDTH - OFFSET_INNER_MID - iw - iw2;
				frameBuffer->paintIcon(NEUTRINO_ICON_SCRAMBLED, icon_x, ypos, item_height);
				iw = iw + iw2 + OFFSET_INNER_MID;
			}
		}

		int numpos = x + OFFSET_INNER_MID + numwidth - g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth(num);
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(numpos, ypos + item_height, numwidth + OFFSET_INNER_SMALL, num, color, item_height);

		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x + OFFSET_INNER_MID + numwidth + OFFSET_INNER_MID, ypos + item_height, width - numwidth - 2*OFFSET_INNER_MID - iw - SCROLLBAR_WIDTH, lname, color);
		//CVFD::getInstance()->showMenuText(0, bouq->channelList->getName(), -1, true);
	}
}

void CBouquetList::paintHead()
{
	std::string icon("");
	CComponentsHeader header(x, y, width, header_height, name, icon, CComponentsHeader::CC_BTN_LEFT | CComponentsHeader::CC_BTN_RIGHT | CComponentsHeader::CC_BTN_MENU);
	header.enableShadow( CC_SHADOW_RIGHT | CC_SHADOW_CORNER_TOP_RIGHT | CC_SHADOW_CORNER_BOTTOM_RIGHT, -1, true);
	header.paint(CC_SAVE_SCREEN_NO);
}

void CBouquetList::paint()
{
	//ensure stop info clock before paint this window
	CInfoClock::getInstance()->disableInfoClock();
	liststart = (selected/listmaxshow)*listmaxshow;
	int lastnum =  liststart + listmaxshow;

	numwidth = 0;
	int maxDigitWidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getMaxDigitWidth();
	int _lastnum = lastnum;
        while (_lastnum) {
                numwidth += maxDigitWidth;
                _lastnum /= 10;
        }

	frameBuffer->paintBoxRel(x, y + header_height, width, height - header_height - footer_height, COL_MENUCONTENT_PLUS_0);

	// no buttons in favonly mode
	int numButtons = (favonly) ? 0 : sizeof(CBouquetListButtons)/sizeof(CBouquetListButtons[0]);

	CComponentsFooter footer;
	footer.enableShadow(CC_SHADOW_ON, -1, true);
	footer.paintButtons(x, y + height - footer_height, width, footer_height, numButtons, CBouquetListButtons);

	if (!Bouquets.empty())
	{
		for (unsigned int count = 0; count < listmaxshow; count++)
		{
			paintItem(count);
		}
	}

	int total_pages;
	int current_page;
	getScrollBarData(&total_pages, &current_page, Bouquets.size(), listmaxshow, selected);
	paintScrollBar(x + width - SCROLLBAR_WIDTH, y + header_height, SCROLLBAR_WIDTH, item_height*listmaxshow, total_pages, current_page, CC_SHADOW_RIGHT_CORNER_ALL);
}
