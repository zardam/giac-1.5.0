// -*- mode:C++ ; compile-command: "g++-3.4 -I. -I.. -g -c Equation.cc -DHAVE_CONFIG_H -DIN_GIAC -Wall" -*-
/*
 *  Copyright (C) 2005,2014 B. Parisse, Institut Fourier, 38402 St Martin d'Heres
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef NUMWORKS
#include "config.h"
#include "giacPCH.h"
#include "kdisplay.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include "input_lexer.h"
#include "input_parser.h"
//giac::context * contextptr=0;
int clip_ymin=0;
int lang=0;
bool xthetat=true;
int esc_flag=0;
using namespace std;
using namespace giac;
const int LCD_WIDTH_PX=320;
const int LCD_HEIGHT_PX=222;

// Numworks Logo commands
#ifndef NO_NAMESPACE_GIAC
namespace giac {
#endif // ndef NO_NAMESPACE_GIAC
  void DefineStatusMessage(const char * s,int a,int b,int c){
  }

  void DisplayStatusArea(){
  }

  void set_xcas_status(){
  }
  int GetSetupSetting(int mode){
    return 0;
  }

  void SetSetupSetting(int mode,int){
  }

  void handle_f5(){
  }

  void delete_clipboard(){}

  bool clip_pasted=true;
  
  string * clipboard(){
    static string * ptr=0;
    if (!ptr)
      ptr=new string;
    return ptr;
  }
  
  void copy_clipboard(const string & s,bool status){
    if (clip_pasted)
      *clipboard()=s;
    else
      *clipboard()+=s;
    clip_pasted=false;
    if (status){
      DefineStatusMessage((char*)(lang?"Selection copiee vers presse-papiers.":"Selection copied to clipboard"), 1, 0, 0);
      DisplayStatusArea();
    }
  }
  
  const char * paste_clipboard(){
    clip_pasted=true;
    return clipboard()->c_str();
  }
  
  int print_msg12(const char * msg1,const char * msg2,int textY=40){
    drawRectangle(0, textY+10, LCD_WIDTH_PX, 44, COLOR_WHITE);
    drawRectangle(3,textY+10,316,3, COLOR_BLACK);
    drawRectangle(3,textY+10,3,44, COLOR_BLACK);
    drawRectangle(316,textY+10,3,44, COLOR_BLACK);
    drawRectangle(3,textY+54,316,3, COLOR_BLACK);
    int textX=30;
    if (msg1)
      numworks_draw_string(textX,textY+15,msg1); //PrintMini(&textX,&textY,(unsigned char*)msg1,0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0); 
    textX=10;
    textY+=35;
    if (msg2)
      numworks_draw_string(textX,textY,msg2);// PrintMini(&textX,&textY,(unsigned char*)msg2,0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0); 
    return textX;
  }
  
  void insert(string & s,int pos,const char * add){
    if (pos>s.size())
      pos=s.size();
    if (pos<0)
      pos=0;
    s=s.substr(0,pos)+add+s.substr(pos,s.size()-pos);
  }
  
  bool do_confirm(const char * s){
    return confirm(s,(lang?"F1: oui,    F6:annuler":"F1: yes,     F6: cancel"))==KEY_CTRL_F1;
  }
  
  int confirm(const char * msg1,const char * msg2,bool acexit){
    int key;
    print_msg12(msg1,msg2);
    while (key!=KEY_CTRL_F1 && key!=KEY_CTRL_F6){
      GetKey(&key);
      if (key==KEY_CTRL_EXE || key==KEY_CTRL_OK)
	key=KEY_CTRL_F1;
      if (key==KEY_CTRL_AC || key==KEY_CTRL_EXIT){
	if (acexit) return -1;
	key=KEY_CTRL_F6;
      }
      set_xcas_status();
    }
    return key;
  }  
  
  bool confirm_overwrite(){
    return do_confirm(lang?"F1: oui,    F6:annuler":"F1: yes,     F6: cancel")==KEY_CTRL_F1;
  }
  
  void invalid_varname(){
    confirm(lang?"Nom de variable incorrect":"Invalid variable name", lang?"F1 ou F6: ok":"F1 or F6: ok");
  }


#ifdef SCROLLBAR
typedef scrollbar TScrollbar;
#endif

#define C24 18 // 24 on 90
#define C18 18 // 18
#define C10 18 // 18
#define C6 6 // 6

  int MB_ElementCount(const char * s){
    return strlen(s); // FIXME for UTF8
  }

  void PrintXY(int x,int y,const char * s,int mode){
    if (mode==TEXT_MODE_NORMAL)
      numworks_giac_draw_string(x,y,giac::_BLACK,giac::_WHITE,s);
    else
      numworks_giac_draw_string(x,y,giac::_WHITE,giac::_BLACK,s);
  }

  void PrintMini(int x,int y,const char * s,int mode){
    if (mode==TEXT_MODE_NORMAL)
      numworks_giac_draw_string_small(x,y,giac::_BLACK,giac::_WHITE,s);
    else
      numworks_giac_draw_string_small(x,y,giac::_WHITE,giac::_BLACK,s);
  }
  
  void printCentered(const char* text, int y) {
    int len = strlen(text);
    int x = LCD_WIDTH_PX/2-(len*6)/2;
    PrintXY(x,y,text,0);
  }
  
int doMenu(Menu* menu, MenuItemIcon* icontable) { // returns code telling what user did. selection is on menu->selection. menu->selection starts at 1!
  int itemsStartY=menu->startY; // char Y where to start drawing the menu items. Having a title increases this by one
  int itemsHeight=menu->height;
  int showtitle = menu->title != NULL;
  if (showtitle) {
    itemsStartY++;
    itemsHeight--;
  }
  char keyword[5];
  keyword[0]=0;
  if(menu->selection > menu->scroll+(menu->numitems>itemsHeight ? itemsHeight : menu->numitems))
    menu->scroll = menu->selection -(menu->numitems>itemsHeight ? itemsHeight : menu->numitems);
  if(menu->selection-1 < menu->scroll)
    menu->scroll = menu->selection -1;
  
  while(1) {
    // Cursor_SetFlashOff();
    if (menu->selection <=1)
      menu->selection=1;
    if (menu->selection > menu->scroll+(menu->numitems>itemsHeight ? itemsHeight : menu->numitems))
      menu->scroll = menu->selection -(menu->numitems>itemsHeight ? itemsHeight : menu->numitems);
    if (menu->selection-1 < menu->scroll)
      menu->scroll = menu->selection -1;
    //if(menu->statusText != NULL) DefineStatusMessage(menu->statusText, 1, 0, 0);
    // Clear the area of the screen we are going to draw on
    if(0 == menu->pBaRtR) {
      int x=C18*(menu->startX-1),
	y=C24*(menu->miniMiniTitle ? itemsStartY:menu->startY),
	w=C18*menu->width*C6+((menu->scrollbar && menu->scrollout)?C6:0),
	h=C24*menu->height-(menu->miniMiniTitle ? C24:0);
      drawRectangle(x, y, w, h, COLOR_WHITE);
      draw_line(x,y,x+w,y,COLOR_BLACK,context0);
      draw_line(x,y+h,x+w,y+h,COLOR_BLACK,context0);
      draw_line(x,y,x,y+h,COLOR_BLACK,context0);
      draw_line(x+w-1,y,x+w-1,y+h,COLOR_BLACK,context0);
    }
    if (menu->numitems>0) {
      for(int curitem=0; curitem < menu->numitems; curitem++) {
        // print the menu item only when appropriate
        if(menu->scroll < curitem+1 && menu->scroll > curitem-itemsHeight) {
          char menuitem[256] = "";
          if(menu->numitems>=100 || menu->type == MENUTYPE_MULTISELECT){
	    strcpy(menuitem, "  "); //allow for the folder and selection icons on MULTISELECT menus (e.g. file browser)
	    strcpy(menuitem+2,menu->items[curitem].text);
	  }
	  else {
	    int cur=curitem+1;
	    if (menu->numitems<10){
	      menuitem[0]='0'+cur;
	      menuitem[1]=' ';
	      menuitem[2]=0;
	      strcpy(menuitem+2,menu->items[curitem].text);
	    }
	    else {
	      menuitem[0]=cur>=10?('0'+(cur/10)):' ';
	      menuitem[1]='0'+(cur%10);
	      menuitem[2]=' ';
	      menuitem[3]=0;
	      strcpy(menuitem+3,menu->items[curitem].text);
	    }
	  }
          //strncat(menuitem, menu->items[curitem].text, 68);
          if(menu->items[curitem].type != MENUITEM_SEPARATOR) {
            //make sure we have a string big enough to have background when item is selected:          
            // MB_ElementCount is used instead of strlen because multibyte chars count as two with strlen, while graphically they are just one char, making fillerRequired become wrong
            int fillerRequired = menu->width - MB_ElementCount(menu->items[curitem].text) - (menu->type == MENUTYPE_MULTISELECT ? 2 : 3);
            for(int i = 0; i < fillerRequired; i++) strcat(menuitem, " ");
            PrintXY(C6*menu->startX,C18*(curitem+itemsStartY-menu->scroll),menuitem, (menu->selection == curitem+1 ? TEXT_MODE_INVERT : TEXT_MODE_NORMAL));
          } else {
            /*int textX = (menu->startX-1) * C18;
            int textY = curitem*C24+itemsStartY*C24-menu->scroll*C24-C24+C6;
            clearLine(menu->startX, curitem+itemsStartY-menu->scroll, (menu->selection == curitem+1 ? textColorToFullColor(menu->items[curitem].color) : COLOR_WHITE));
            drawLine(textX, textY+C24-4, LCD_WIDTH_PX-2, textY+C24-4, COLOR_GRAY);
            PrintMini(&textX, &textY, (unsigned char*)menuitem, 0, 0xFFFFFFFF, 0, 0, (menu->selection == curitem+1 ? COLOR_WHITE : textColorToFullColor(menu->items[curitem].color)), (menu->selection == curitem+1 ? textColorToFullColor(menu->items[curitem].color) : COLOR_WHITE), 1, 0);*/
          }
          // deal with menu items of type MENUITEM_CHECKBOX
          if(menu->items[curitem].type == MENUITEM_CHECKBOX) {
            PrintXY(C6*(menu->startX+menu->width-1),C18*(curitem+itemsStartY-menu->scroll),
              (menu->items[curitem].value == MENUITEM_VALUE_CHECKED ? "\xe6\xa9" : "\xe6\xa5"),
              (menu->selection == curitem+1 ? TEXT_MODE_INVERT : (menu->pBaRtR == 1? TEXT_MODE_NORMAL : TEXT_MODE_NORMAL)));
          }
          // deal with multiselect menus
          if(menu->type == MENUTYPE_MULTISELECT) {
            if((curitem+itemsStartY-menu->scroll)>=itemsStartY &&
              (curitem+itemsStartY-menu->scroll)<=(itemsStartY+itemsHeight) &&
              icontable != NULL
            ) {
#if 0
              if (menu->items[curitem].isfolder == 1) {
                // assumes first icon in icontable is the folder icon
                CopySpriteMasked(icontable[0].data, (menu->startX)*C18, (curitem+itemsStartY-menu->scroll)*C24, 0x12, 0x18, 0xf81f  );
              } else {
                if(menu->items[curitem].icon >= 0) CopySpriteMasked(icontable[menu->items[curitem].icon].data, (menu->startX)*C18, (curitem+itemsStartY-menu->scroll)*C24, 0x12, 0x18, 0xf81f  );
              }
#endif
            }
            if (menu->items[curitem].isselected) {
              if (menu->selection == curitem+1) {
                PrintXY(C6*menu->startX,C18*(curitem+itemsStartY-menu->scroll),"\xe6\x9b", TEXT_MODE_NORMAL);
              } else {
                PrintXY(C6*menu->startX,C18*(curitem+itemsStartY-menu->scroll),"\xe6\x9b", TEXT_MODE_NORMAL);
              }
            }
          }
        }
      }
      if (menu->scrollbar) {
#ifdef SCROLLBAR
        TScrollbar sb;
        sb.I1 = 0;
        sb.I5 = 0;
        sb.indicatormaximum = menu->numitems;
        sb.indicatorheight = itemsHeight;
        sb.indicatorpos = menu->scroll;
        sb.barheight = itemsHeight*C24;
        sb.bartop = (itemsStartY-1)*C24;
        sb.barleft = menu->startX*C18+menu->width*C18 - C18 - (menu->scrollout ? 0 : 5);
        sb.barwidth = C6;
        Scrollbar(&sb);
#endif
      }
      //if(menu->type==MENUTYPE_MULTISELECT && menu->fkeypage == 0) drawFkeyLabels(0x0037); // SELECT (white)
    } else {
      giac::printCentered(menu->nodatamsg, (itemsStartY*C24)+(itemsHeight*C24)/2-12);
    }
    if(showtitle) {
      if(menu->miniMiniTitle) {
        int textX = 0, textY=(menu->startY-1)*C24;
        PrintMini( textX, textY, menu->title, 0 );
      } else PrintXY(6*menu->startX, 1+C18*menu->startY, menu->title, TEXT_MODE_NORMAL);
      if(menu->subtitle != NULL) {
        int textX=(MB_ElementCount(menu->title)+menu->startX-1)*C18+C10, textY=C6;
        PrintMini(textX, textY, menu->subtitle, 0);
      }
      PrintXY(278, 1, "____", 0);
      PrintXY(278, 1, keyword, 0);
    }
    /*if(menu->darken) {
      DrawFrame(COLOR_BLACK);
      VRAMInvertArea(menu->startX*C18-C18, menu->startY*C24, menu->width*C18-(menu->scrollout || !menu->scrollbar ? 0 : 5), menu->height*C24);
    }*/
    if(menu->type == MENUTYPE_NO_KEY_HANDLING) return MENU_RETURN_INSTANT; // we don't want to handle keys
    int key;
    GetKey(&key);
    if (key<256 && isalpha(key)){
      key=tolower(key);
      int pos=strlen(keyword);
      if (pos>=4)
	pos=0;
      keyword[pos]=key;
      keyword[pos+1]=0;
      int cur=0;
      for (;cur<menu->numitems;++cur){
#if 1
	if (strcmp(menu->items[cur].text,keyword)>=0)
	  break;
#else
	char c=menu->items[cur].text[0];
	if (key<=c)
	  break;
#endif
      }
      if (cur<menu->numitems){
	menu->selection=cur+1;
	if(menu->selection > menu->scroll+(menu->numitems>itemsHeight ? itemsHeight : menu->numitems))
	  menu->scroll = menu->selection -(menu->numitems>itemsHeight ? itemsHeight : menu->numitems);
	if(menu->selection-1 < menu->scroll)
	  menu->scroll = menu->selection -1;
      }
      continue;
    }
    switch(key) {
    case KEY_CTRL_PAGEDOWN:
      menu->selection+=6;
      if (menu->selection >= menu->numitems)
	menu->selection=menu->numitems;
      if(menu->selection > menu->scroll+(menu->numitems>itemsHeight ? itemsHeight : menu->numitems))
	menu->scroll = menu->selection -(menu->numitems>itemsHeight ? itemsHeight : menu->numitems);
      break;
    case KEY_CTRL_DOWN:
      if(menu->selection == menu->numitems)
	{
          if(menu->returnOnInfiniteScrolling) {
            return MENU_RETURN_SCROLLING;
          } else {
            menu->selection = 1;
            menu->scroll = 0;
          }
        }
      else
        {
	  menu->selection++;
          if(menu->selection > menu->scroll+(menu->numitems>itemsHeight ? itemsHeight : menu->numitems))
            menu->scroll = menu->selection -(menu->numitems>itemsHeight ? itemsHeight : menu->numitems);
        }
      if(menu->pBaRtR==1) return MENU_RETURN_INSTANT;
      break;
    case KEY_CTRL_PAGEUP:
      menu->selection-=6;
      if (menu->selection <=1)
	menu->selection=1;
      if(menu->selection-1 < menu->scroll)
	menu->scroll = menu->selection -1;
      break;
    case KEY_CTRL_UP:
      if(menu->selection == 1)
	{
	  if(menu->returnOnInfiniteScrolling) {
	    return MENU_RETURN_SCROLLING;
	  } else {
	    menu->selection = menu->numitems;
	    menu->scroll = menu->selection-(menu->numitems>itemsHeight ? itemsHeight : menu->numitems);
	  }
	}
      else
	{
	  menu->selection--;
	  if(menu->selection-1 < menu->scroll)
	    menu->scroll = menu->selection -1;
	}
      if(menu->pBaRtR==1) return MENU_RETURN_INSTANT;
      break;
    case KEY_CTRL_F1:
      if(menu->type==MENUTYPE_MULTISELECT && menu->fkeypage == 0 && menu->numitems > 0) {
          /*if(menu->items[menu->selection-1].isselected) {
            menu->items[menu->selection-1].isselected=0;
            menu->numselitems = menu->numselitems-1;
	    } else {
            menu->items[menu->selection-1].isselected=1;
            menu->numselitems = menu->numselitems+1;
	    }
	    return key; //return on F1 too so that parent subroutines have a chance to e.g. redraw fkeys*/
      } else if (menu->type == MENUTYPE_FKEYS) {
	return key;
      }
      break;
    case KEY_CTRL_F2:
    case KEY_CTRL_F3:
    case KEY_CTRL_F4:
    case KEY_CTRL_F5:
    case KEY_CTRL_F6: case KEY_CTRL_CATALOG:
    case KEY_CHAR_ANS:
      if (menu->type == MENUTYPE_FKEYS || menu->type==MENUTYPE_MULTISELECT) return key; // MULTISELECT also returns on Fkeys
      break;
    case KEY_CTRL_PASTE:
      if (menu->type==MENUTYPE_MULTISELECT) return key; // MULTISELECT also returns on paste
    case KEY_CTRL_OPTN:
      if (menu->type==MENUTYPE_FKEYS || menu->type==MENUTYPE_MULTISELECT) return key;
      break;
    case KEY_CTRL_FORMAT:
      if (menu->type==MENUTYPE_FKEYS) return key; // return on the Format key so that event lists can prompt to change event category
      break;
    case KEY_CTRL_RIGHT:
      if(menu->type != MENUTYPE_MULTISELECT) break;
      // else fallthrough
    case KEY_CTRL_EXE: case KEY_CTRL_OK:
      if(menu->numitems>0) return key==KEY_CTRL_OK?MENU_RETURN_SELECTION:key;
      break;
    case KEY_CTRL_LEFT:
      if(menu->type != MENUTYPE_MULTISELECT) break;
      // else fallthrough
    case KEY_CTRL_DEL:
      if (strlen(keyword))
	keyword[strlen(keyword)-1]=0;
      break;
    case KEY_CTRL_AC:
      if (strlen(keyword)){
	keyword[0]=0;
	//SetSetupSetting( (unsigned int)0x14, 0x88);	
	//DisplayStatusArea();
	break;
      }
    case KEY_CTRL_EXIT: 
      return MENU_RETURN_EXIT;
      break;
    case KEY_CHAR_1:
    case KEY_CHAR_2:
    case KEY_CHAR_3:
    case KEY_CHAR_4:
    case KEY_CHAR_5:
    case KEY_CHAR_6:
    case KEY_CHAR_7:
    case KEY_CHAR_8:
    case KEY_CHAR_9:
      if(menu->numitems>=(key-0x30)) {
	menu->selection = (key-0x30);
	if (menu->type != MENUTYPE_FKEYS) return MENU_RETURN_SELECTION;
      }
      break;
    case KEY_CHAR_0:
      if(menu->numitems>=10) {
	menu->selection = 10;
	if (menu->type != MENUTYPE_FKEYS)  return MENU_RETURN_SELECTION;
      }
      break;
    case KEY_CTRL_XTT:
      if(menu->numitems>=11) {
	menu->selection = 11;
	if (menu->type != MENUTYPE_FKEYS)  return MENU_RETURN_SELECTION;
      }
      break;
    case KEY_CHAR_LOG:
      if(menu->numitems>=12) {
	menu->selection = 12;
	if (menu->type != MENUTYPE_FKEYS)  return MENU_RETURN_SELECTION;
      }
      break;
    case KEY_CHAR_LN:
      if(menu->numitems>=13) {
	menu->selection = 13;
	if (menu->type != MENUTYPE_FKEYS)  return MENU_RETURN_SELECTION;
      }
      break;
    case KEY_CHAR_SIN:
    case KEY_CHAR_COS:
    case KEY_CHAR_TAN:
      if(menu->numitems>=(key-115)) {
	menu->selection = (key-115);
	if (menu->type != MENUTYPE_FKEYS)  return MENU_RETURN_SELECTION;
      }
      break;
    case KEY_CHAR_FRAC:
      if(menu->numitems>=17) {
	menu->selection = 17;
	if (menu->type != MENUTYPE_FKEYS)  return MENU_RETURN_SELECTION;
      }
      break;
    case KEY_CTRL_FD:
      if(menu->numitems>=18) {
	menu->selection = 18;
	if (menu->type != MENUTYPE_FKEYS)  return MENU_RETURN_SELECTION;
      }
      break;
    case KEY_CHAR_LPAR:
    case KEY_CHAR_RPAR:
      if(menu->numitems>=(key-21)) {
	menu->selection = (key-21);
	if (menu->type != MENUTYPE_FKEYS)  return MENU_RETURN_SELECTION;
      }
      break;
    case KEY_CHAR_COMMA:
      if(menu->numitems>=21) {
	menu->selection = 21;
	if (menu->type != MENUTYPE_FKEYS)  return MENU_RETURN_SELECTION;
      }
      break;
    case KEY_CHAR_STORE:
      if(menu->numitems>=22) {
	menu->selection = 22;
	if (menu->type != MENUTYPE_FKEYS)  return MENU_RETURN_SELECTION;
      }
      break;
    }
  }
  return MENU_RETURN_EXIT;
}

void reset_alpha(){
  SetSetupSetting( (unsigned int)0x14, 0);	
  //DisplayStatusArea();
}

#define CAT_CATEGORY_ALL 0
#define CAT_CATEGORY_ALGEBRA 1
#define CAT_CATEGORY_LINALG 2
#define CAT_CATEGORY_CALCULUS 3
#define CAT_CATEGORY_ARIT 4
#define CAT_CATEGORY_COMPLEXNUM 5
#define CAT_CATEGORY_PLOT 6
#define CAT_CATEGORY_POLYNOMIAL 7
#define CAT_CATEGORY_PROBA 8
#define CAT_CATEGORY_PROGCMD 9
#define CAT_CATEGORY_REAL 10
#define CAT_CATEGORY_SOLVE 11
#define CAT_CATEGORY_STATS 12
#define CAT_CATEGORY_TRIG 13
#define CAT_CATEGORY_OPTIONS 14
#define CAT_CATEGORY_LIST 15
#define CAT_CATEGORY_MATRIX 16
#define CAT_CATEGORY_PROG 17
#define CAT_CATEGORY_SOFUS 18
#define CAT_CATEGORY_LOGO 19 // should be the last one

void init_locale(){
  lang=1;
}

const catalogFunc completeCat[] = { // list of all functions (including some not in any category)
  // {"cosh(x)", 0, "Hyperbolic cosine of x.", 0, 0, CAT_CATEGORY_TRIG},
  // {"exp(x)", 0, "Renvoie e^x.", "1.2", 0, CAT_CATEGORY_REAL},
  // {"exponential_regression_plot(Xlist,Ylist)", 0, "Graphe d'une regression exponentielle.", "#X,Y:=[1,2,3,4,5],[0,1,3,4,4];exponential_regression_plot(X,Y);scatterplot(X,Y)", 0, CAT_CATEGORY_STATS},
  // {"log(x)", 0, "Logarithme naturel de x.", 0, 0, CAT_CATEGORY_REAL},
  // {"logarithmic_regression_plot(Xlist,Ylist)", 0, "Graphe d'une regression logarithmique.", "#X,Y:=[1,2,3,4,5],[0,1,3,4,4];logarithmic_regression_plot(X,Y);scatterplot(X,Y)", 0, CAT_CATEGORY_STATS},
  // {"polynomial_regression_plot(Xlist,Ylist,n)", 0, "Graphe d'une regression polynomiale de degre <= n.", "#X,Y:=[1,2,3,4,5],[0,1,3,4,4];polynomial_regression_plot(X,Y,2);scatterplot(X,Y)", 0, CAT_CATEGORY_STATS},
  // {"power_regression_plot(Xlist,Ylist,n)", 0, "Graphe d'une regression puissance.", "#X,Y:=[1,2,3,4,5],[0,1,3,4,4];power_regression_plot(X,Y);scatterplot(X,Y)", 0, CAT_CATEGORY_STATS},
  // {"sinh(x)", 0, "Hyperbolic sine of x.", 0, 0, CAT_CATEGORY_TRIG},
  // {"tanh(x)", 0, "Hyperbolic tangent of x.", 0, 0, CAT_CATEGORY_TRIG},
  {" boucle for (pour)", "for ", "Boucle definie pour un indice variant entre 2 valeurs fixees", "#\nfor ", 0, CAT_CATEGORY_PROG},
  {" boucle liste", "for in", "Boucle sur tous les elements d'une liste.", "#\nfor in", 0, CAT_CATEGORY_PROG},
  {" boucle while (tantque)", "while ", "Boucle indefinie tantque.", "#\nwhile ", 0, CAT_CATEGORY_PROG},
  {" test si alors", "if ", "Test", "#\nif ", 0, CAT_CATEGORY_PROG},
  {" test sinon", "else ", "Clause fausse du test", 0, 0, CAT_CATEGORY_PROG},
  {" fonction def.", "f(x):=", "Definition de fonction.", "#\nf(x):=", 0, CAT_CATEGORY_PROG},
  {" local j,k;", "local ", "Declaration de variables locales Xcas", 0, 0, CAT_CATEGORY_PROG},
  {" range(a,b)", "in range(", "Dans l'intervalle [a,b[ (a inclus, b exclus)", "# in range(1,10)", 0, CAT_CATEGORY_PROG},
  {" return res;", "return ", "return ou retourne quitte la fonction et renvoie le resultat res", 0, 0, CAT_CATEGORY_PROG},
  {" edit list ", "list ", "Assistant creation de liste.", 0, 0, CAT_CATEGORY_LIST},
  {" edit matrix ", "matrix ", "Assistant creation de matrice.", 0, 0, CAT_CATEGORY_MATRIX},
  //{"fonction def Xcas", "fonction f(x) local y;   ffonction:;", "Definition de fonction.", "#fonction f(x) local y; y:=x^2; return y; ffonction:;", 0, CAT_CATEGORY_PROG},
  {"!", "!", "Non logique (prefixe) ou factorielle de n (suffixe). Raccourci shift F1", "#7!", "#!b", CAT_CATEGORY_PROGCMD},
  {"#", "#", "Commentaire Python, en Xcas taper //. Raccourci ALPHA F2", 0, 0, CAT_CATEGORY_PROG},
  {"%", "%", "a % b signifie a modulo b", 0, 0, CAT_CATEGORY_ARIT | (CAT_CATEGORY_PROGCMD << 8)},
  {"&", "&", "Et logique ou +", "#1&2", 0, CAT_CATEGORY_PROGCMD},
  {":=", ":=", "Affectation vers la gauche (inverse de =>). Raccourci SHIFT F1", "#a:=3", 0, CAT_CATEGORY_PROGCMD|(CAT_CATEGORY_SOFUS<<8)},
  {"<", "<", "Inferieur strict. Raccourci SHIFT F2", 0, 0, CAT_CATEGORY_PROGCMD},
  {"=>", "=>", "Affectation vers la droite ou conversion en (touche ->). Par exemple 5=>a ou x^4-1=>* ou (x+1)^2=>+ ou sin(x)^2=>cos.", "#5=>a", 0, CAT_CATEGORY_PROGCMD},
  {">", ">", "Superieur strict. Raccourci F2.", 0, 0, CAT_CATEGORY_PROGCMD},
  {"\\", "\\", "Caractere \\", 0, 0, CAT_CATEGORY_PROGCMD},
  {"_", "_", "Caractere _. Raccourci (-).", 0, 0, CAT_CATEGORY_PROGCMD},
  {"_cdf", "_cdf", "Suffixe pour obtenir une distribution cumulee. Taper F2 pour la distribution cumulee inverse.", "#_icdf", 0, CAT_CATEGORY_PROBA},
  {"_plot", "_plot", "Suffixe pour obtenir le graphe d'une regression.", "#X,Y:=[1,2,3,4,5],[0,1,3,4,4];polynomial_regression_plot(X,Y,2);scatterplot(X,Y)", 0, CAT_CATEGORY_STATS},
  {"a and b", " and ", "Et logique", 0, 0, CAT_CATEGORY_PROGCMD},
  {"a or b", " or ", "Ou logique", 0, 0, CAT_CATEGORY_PROGCMD},
  {"abcuv(a,b,c)", 0, "Cherche 2 polynomes u,v tels que a*u+b*v=c","x+1,x^2-2,x", 0, CAT_CATEGORY_POLYNOMIAL},
  {"abs(x)", 0, "Valeur absolue, module ou norme de x", "-3", "[1,2,3]", CAT_CATEGORY_COMPLEXNUM | (CAT_CATEGORY_REAL<<8)},
  {"append", 0, "Ajoute un element en fin de liste l","#l.append(x)", 0, CAT_CATEGORY_LIST},
  {"approx(x)", 0, "Valeur approchee de x. Raccourci S-D", "pi", 0, CAT_CATEGORY_REAL},
  {"arg(z)", 0, "Argument du complexe z.", "1+i", 0, CAT_CATEGORY_COMPLEXNUM},
  {"asc(string)", 0, "Liste des codes ASCII d'une chaine", "\"Bonjour\"", 0, CAT_CATEGORY_ARIT},
  {"assume(hyp)", 0, "Hypothese sur une variable.", "x>1", "x>-1 and x<1", CAT_CATEGORY_PROGCMD | (CAT_CATEGORY_SOFUS<<8)},
  {"avance n", "avance ", "La tortue avance de n pas, par defaut n=10", "#avance 40", 0, CAT_CATEGORY_LOGO},
  {"axes", "axes", "Axes visibles ou non axes=1 ou 0", "#axes=0", "#axes=1", CAT_CATEGORY_PROGCMD << 8},
  {"baisse_crayon ", "baisse_crayon ", "La tortue se deplace en marquant son passage.", 0, 0, CAT_CATEGORY_LOGO},
  {"barplot(list)", 0, "Diagramme en batons d'une serie statistique 1d.", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_STATS},
  {"binomial(n,p,k)", 0, "binomial(n,p,k) probabilite de k succes avec n essais ou p est la proba de succes d'un essai. binomial_cdf(n,p,k) est la probabilite d'obtenir au plus k succes avec n essais. binomial_icdf(n,p,t) renvoie le plus petit k tel que binomial_cdf(n,p,k)>=t", "10,.5,4", 0, CAT_CATEGORY_PROBA},
  {"bitxor", "bitxor", "Ou exclusif", "#bitxor(1,2)", 0, CAT_CATEGORY_PROGCMD},
  {"black", "black", "Option d'affichage", "#display=black", 0, CAT_CATEGORY_PROGCMD},
  {"blue", "blue", "Option d'affichage", "#display=blue", 0, CAT_CATEGORY_PROGCMD},
  {"cache_tortue ", "cache_tortue ", "Cache la tortue apres avoir trace le dessin.", 0, 0, CAT_CATEGORY_LOGO},
  {"camembert(list)", 0, "Diagramme en camembert d'une serie statistique 1d.", "[[\"France\",6],[\"Allemagne\",12],[\"Suisse\",5]]", 0, CAT_CATEGORY_STATS},
  {"ceiling(x)", 0, "Partie entiere superieure", "1.2", 0, CAT_CATEGORY_REAL},
  {"cercle(centre,rayon)", 0, "Cercle donne par centre et rayon ou par un diametre", "2+i,3", "1-i,1+i", CAT_CATEGORY_PROGCMD},
  {"cfactor(p)", 0, "Factorisation sur C.", "x^4-1", 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_COMPLEXNUM << 8)},
  {"char(liste)", 0, "Chaine donnee par une liste de code ASCII", "[97,98,99]", 0, CAT_CATEGORY_ARIT},
  {"charpoly(M,x)", 0, "Polynome caracteristique de la matrice M en la variable x.", "[[1,2],[3,4]],x", 0, CAT_CATEGORY_MATRIX},
  {"clearscreen()", "clearscreen()", "Efface l'ecran.", 0, 0, CAT_CATEGORY_PROGCMD},
  {"coeff(p,x,n)", 0, "Coefficient de x^n dans le polynome p.", "(1+x)^6,x,3", 0, CAT_CATEGORY_POLYNOMIAL},
  {"comb(n,k)", 0, "Renvoie k parmi n.", "10,4", 0, CAT_CATEGORY_PROBA},
  {"cond(A,[1,2,inf])", 0, "Nombre de condition d'une matrice par rapport a la norme specifiee (par defaut 1)", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
  {"conj(z)", 0, "Conjugue complexe de z.", "1+i", 0, CAT_CATEGORY_COMPLEXNUM},
  {"correlation(l1,l2)", 0, "Correlation listes l1 et l2", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
  {"covariance(l1,l2)", 0, "Covariance listes l1 et l2", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
  {"cpartfrac(p,x)", 0, "Decomposition en elements simples sur C.", "1/(x^4-1)", 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_COMPLEXNUM << 8)},
  {"crayon ", "crayon ", "Couleur de trace de la tortue", "#crayon rouge", 0, CAT_CATEGORY_LOGO},
  {"cross(u,v)", 0, "Produit vectoriel de u et v.","[1,2,3],[0,1,3]", 0, CAT_CATEGORY_LINALG},
  {"csolve(equation,x)", 0, "Resolution exacte dans C d'une equation en x (ou d'un systeme polynomial).","x^2+x+1=0", 0, CAT_CATEGORY_SOLVE | (CAT_CATEGORY_COMPLEXNUM << 8)},
  {"curl(u,vars)", 0, "Rotationnel du vecteur u.", "[2*x*y,x*z,y*z],[x,y,z]", 0, CAT_CATEGORY_LINALG},
  {"cyan", "cyan", "Option d'affichage", "#display=cyan", 0, CAT_CATEGORY_PROGCMD},
  {"debug(f(args))", 0, "Execute la fonction f en mode pas a pas.", 0, 0, CAT_CATEGORY_PROG},
  {"degree(p,x)", 0, "Degre du polynome p en x.", "x^4-1", 0, CAT_CATEGORY_POLYNOMIAL},
  {"denom(x)", 0, "Denominateur de l'expression x.", "3/4", 0, CAT_CATEGORY_POLYNOMIAL},
  {"desolve(equation,t,y)", 0, "Resolution exacte d'equation differentielle ou de systeme differentiel lineaire a coefficients constants.", "[y'+y=exp(x),y(0)=1]", "[y'=[[1,2],[2,1]]*y+[x,x+1],y(0)=[1,2]]", CAT_CATEGORY_SOLVE | (CAT_CATEGORY_CALCULUS << 8)},
  {"det(A)", 0, "Determinant de la matrice A.", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
  {"diff(f,var,[n])", 0, "Derivee de l'expression f par rapport a var (a l'ordre n, n=1 par defaut), par exemple diff(sin(x),x) ou diff(x^3,x,2). Pour deriver f par rapport a x, utiliser f' (raccourci F3). Pour le gradient de f, var est la liste des variables.", "sin(x),x", "sin(x^2),x,3", CAT_CATEGORY_CALCULUS},
  {"display", "display", "Option d'affichage", "#display=red", 0, CAT_CATEGORY_PROGCMD},
  {"disque n", "disque ", "Cercle rempli tangent a la tortue, de rayon n. Utiliser disque n,theta pour remplir un morceau de camembert ou disque n,theta,segment pour remplir un segment de disque", "#disque 30", "#disque(30,90)", CAT_CATEGORY_LOGO},
  {"dot(a,b)", 0, "Produit scalaire de 2 vecteurs. Raccourci: *", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_LINALG},
  {"draw_arc(x1,y1,rx,ry,theta1,theta2,c)", 0, "Arc d'ellipse pixelise.", "100,100,60,80,0,pi,magenta", 0, CAT_CATEGORY_PROGCMD},
  {"draw_circle(x1,y1,r,c)", 0, "Cercle pixelise. Option filled pour le remplir.", "100,100,60,cyan+filled", 0, CAT_CATEGORY_PROGCMD},
  {"draw_line(x1,y1,x2,y2,c)", 0, "Droite pixelisee.", "100,50,300,200,blue", 0, CAT_CATEGORY_PROGCMD},
  {"draw_pixel(x,y,color)", 0, "Colorie le pixel x,y. Faire draw_pixel() pour synchroniser l'ecran.", 0, 0, CAT_CATEGORY_PROGCMD},
  {"draw_polygon([[x1,y1],...],c)", 0, "Polygone pixelise.", "[[100,50],[30,20],[60,70]],red+filled", 0, CAT_CATEGORY_PROGCMD},
  {"draw_rectangle(x,y,w,h,c)", 0, "Rectangle pixelise.", "100,50,30,20,red+filled", 0, CAT_CATEGORY_PROGCMD},
  {"draw_string(s,x,y,c)", 0, "Affiche la chaine s en x,y", "\"Bonjour\",80,60", 0, CAT_CATEGORY_PROGCMD},
  {"droite(equation)", 0, "Droite donnee par une equation ou 2 points", "y=2x+1", "1+i,2-i", CAT_CATEGORY_PROGCMD},
  {"ecris ", "ecris ", "Ecrire a la position de la tortue", "#ecris \"coucou\"", 0, CAT_CATEGORY_LOGO},
  {"efface", "efface", "Remise a zero de la tortue", 0, 0, CAT_CATEGORY_LOGO},
  {"egcd(A,B)", 0, "Cherche des polynomes U,V,D tels que A*U+B*V=D=gcd(A,B)","x^2+3x+1,x^2-5x-1", 0, CAT_CATEGORY_POLYNOMIAL},
  {"eigenvals(A)", 0, "Valeurs propres de la matrice A.", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
  {"eigenvects(A)", 0, "Vecteurs propres de la matrice A.", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
  {"elif (test)", "elif", "Tests en cascade", 0, 0, CAT_CATEGORY_PROG},
  //{"end", "end", "Fin de bloc", 0, 0, CAT_CATEGORY_PROG},
  {"erf(x)", 0, "Fonction erreur en x.", "1.2", 0, CAT_CATEGORY_PROBA},
  {"erfc(x)", 0, "Fonction erreur complementaire en x.", "1.2", 0, CAT_CATEGORY_PROBA},
  {"euler(n)",0,"Indicatrice d'Euler: nombre d'entiers < n premiers avec n","25",0,CAT_CATEGORY_ARIT},
  {"eval(f)", 0, "Evalue f.", 0, 0, CAT_CATEGORY_PROGCMD},
  {"evalc(z)", 0, "Ecrit z=x+i*y.", "1/(1+i*sqrt(3))", 0, CAT_CATEGORY_COMPLEXNUM},
  {"exact(x)", 0, "Convertit x en rationnel. Raccourci shift S-D", "1.2", 0, CAT_CATEGORY_REAL},
  {"exp2trig(expr)", 0, "Conversion d'exponentielles complexes en sin/cos", "exp(i*x)", 0, CAT_CATEGORY_TRIG},
  {"exponential_regression(Xlist,Ylist)", 0, "Regression exponentielle.", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
  {"exponentiald(lambda,x)", 0, "Loi exponentielle de parametre lambda. exponentiald_cdf(lambda,x) probabilite que \"loi exponentielle <=x\" par ex. exponentiald_cdf(2,3). exponentiald_icdf(lambda,t) renvoie x tel que \"loi exponentielle <=x\" vaut t, par ex. exponentiald_icdf(2,0.95) ", "5.1,3.4", 0, CAT_CATEGORY_PROBA},
  {"extend", 0, "Concatene 2 listes. Attention a ne pas utiliser + qui effectue l'addition de 2 vecteurs.","#l1.extend(l2)", 0, CAT_CATEGORY_LIST},
  {"factor(p,[x])", 0, "Factorisation du polynome p (utiliser ifactor pour un entier). Raccourci: p=>*", "x^4-1", "x^6+1,sqrt(3)", CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"filled", "filled", "Option d'affichage", 0, 0, CAT_CATEGORY_PROGCMD},
  {"float(x)", 0, "Convertit x en nombre approche (flottant).", "pi", 0, CAT_CATEGORY_REAL},
  {"floor(x)", 0, "Partie entiere de x", "pi", 0, CAT_CATEGORY_REAL},
  {"fonction f(x)", "fonction", "Definition de fonction (Xcas). Par exemple\nfonction f(x)\n local y;\ny:=x*x;\nreturn y;\nffonction", 0, 0, CAT_CATEGORY_PROG},
  {"from math/... import *", "from math import *", "Instruction pour utiliser les fonctions de maths ou des fonctions aleatoires [random] ou la tortue en anglais [turtle]. Importer math n'est pas necessaire dans KhiCAS", "#from random import *", "#from turtle import *", CAT_CATEGORY_PROG},
  {"fsolve(equation,x=a[..b])", 0, "Resolution approchee de equation pour x dans l'intervalle a..b ou en partant de x=a.","cos(x)=x,x=0..1", "cos(x)-x,x=0.0", CAT_CATEGORY_SOLVE},
  {"gcd(a,b,...)", 0, "Plus grand commun diviseur. Voir iegcd ou egcd pour Bezout.", "23,13", "x^2-1,x^3-1", CAT_CATEGORY_ARIT | (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"gl_x", "gl_x", "Reglage graphique X gl_x=xmin..xmax", "#gl_x=0..2", 0, CAT_CATEGORY_PROGCMD << 8},
  {"gl_y", "gl_y", "Reglage graphique Y gl_y=ymin..ymax", "#gl_y=-1..1", 0, CAT_CATEGORY_PROGCMD << 8},
  {"green", "green", "Option d'affichage", "#display=green", 0, CAT_CATEGORY_PROGCMD},
  {"halftan(expr)", 0, "Exprime cos, sin, tan avec tan(angle/2).","cos(x)", 0, CAT_CATEGORY_TRIG},
  {"hermite(n)", 0, "n-ieme polynome de Hermite", "10", "10,t", CAT_CATEGORY_POLYNOMIAL},
  {"hilbert(n)", 0, "Matrice de Hilbert de taille n.", "4", 0, CAT_CATEGORY_MATRIX},
  {"histogram(list,min,size)", 0, "Histogramme d'une liste de donneees, classes commencant a min de taille size.","ranv(100,uniformd,0,1),0,0.1", 0, CAT_CATEGORY_STATS},
  {"iabcuv(a,b,c)", 0, "Cherche 2 entiers u,v tels que a*u+b*v=c","23,13,15", 0, CAT_CATEGORY_ARIT},
  {"ichinrem([a,m],[b,n])", 0,"Restes chinois entiers de a mod m et b mod n.", "[3,13],[2,7]", 0, CAT_CATEGORY_ARIT},
  {"idivis(n)", 0, "Liste des diviseurs d'un entier n.", "10", 0, CAT_CATEGORY_ARIT},
  {"idn(n)", 0, "matrice identite n * n", "4", 0, CAT_CATEGORY_MATRIX},
  {"iegcd(a,b)", 0, "Determine les entiers u,v,d tels que a*u+b*v=d=gcd(a,b)","23,13", 0, CAT_CATEGORY_ARIT},
  {"ifactor(n)", 0, "Factorisation d'un entier (pas trop grand!). Raccourci n=>*", "1234", 0, CAT_CATEGORY_ARIT},
  {"ilaplace(f,s,x)", 0, "Transformee inverse de Laplace de f", "s/(s^2+1),s,x", 0, CAT_CATEGORY_CALCULUS},
  {"im(z)", 0, "Partie imaginaire.", "1+i", 0, CAT_CATEGORY_COMPLEXNUM},
  {"inf", "inf", "Plus l'infini. Utiliser -inf pour moins l'infini ou infinity pour l'infini complexe. Raccourci shift INS.", "-inf", "infinity", CAT_CATEGORY_CALCULUS},
  {"input()", "input()", "Lire une chaine au clavier", "\"Valeur ?\"", 0, CAT_CATEGORY_PROG},
  {"integrate(f,x,[,a,b])", 0, "Primitive de f par rapport a la variable x, par ex. integrate(x*sin(x),x). Pour calculer une integrale definie, entrer les arguments optionnels a et b, par ex. integrate(x*sin(x),x,0,pi). Raccourci SHIFT F3.", "x*sin(x),x", "cos(x)/(1+x^4),x,0,inf", CAT_CATEGORY_CALCULUS},
  {"interp(X,Y[,interp])", 0, "Interpolation de Lagrange aux points (xi,yi) avec X la liste des xi et Y des yi. Renvoie la liste des differences divisees si interp est passe en parametre.", "[1,2,3,4,5],[0,1,3,4,4]", "[1,2,3,4,5],[0,1,3,4,4],interp", CAT_CATEGORY_POLYNOMIAL},
  {"inv(A)", 0, "Inverse de A.", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
  {"inverser(v)", "inverser ", "La variable v est remplacee par son inverse", "#v:=3; inverser v", 0, CAT_CATEGORY_SOFUS},
  {"iquo(a,b)", 0, "Quotient euclidien de deux entiers.", "23,13", 0, CAT_CATEGORY_ARIT},
  {"irem(a,b)", 0,"Reste euclidien de deux entiers", "23,13", 0, CAT_CATEGORY_ARIT},
  {"isprime(n)", 0, "Renvoie 1 si n est premier, 0 sinon.", "11", "10", CAT_CATEGORY_ARIT},
  {"jordan(A)", 0, "Forme normale de Jordan de la matrice A, renvoie P et D tels que P^-1*A*P=D", "[[1,2],[3,4]]", "[[1,1,-1,2,-1],[2,0,1,-4,-1],[0,1,1,1,1],[0,1,2,0,1],[0,0,-3,3,-1]]", CAT_CATEGORY_MATRIX},
  {"laguerre(n,a,x)", 0, "n-ieme polynome de Laguerre (a=0 par defaut).", "10", 0, CAT_CATEGORY_POLYNOMIAL},
  {"laplace(f,x,s)", 0, "Transformee de Laplace de f","sin(x),x,s", 0, CAT_CATEGORY_CALCULUS},
  {"lcm(a,b,...)", 0, "Plus petit commun multiple.", "23,13", "x^2-1,x^3-1", CAT_CATEGORY_ARIT | (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"lcoeff(p,x)", 0, "Coefficient dominant du polynome p.", "x^4-1", 0, CAT_CATEGORY_POLYNOMIAL},
  {"legendre(n)", 0, "n-ieme polynome de Legendre.", "10", "10,t", CAT_CATEGORY_POLYNOMIAL},
#ifdef RELEASE
  {"len(l)", 0, "Taille d'une liste.", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_LIST},
#endif
  {"leve_crayon ", "leve_crayon ", "La tortue se deplace sans marquer son passage", 0, 0, CAT_CATEGORY_LOGO},
  {"limit(f,x=a)", 0, "Limite de f en x = a. Ajouter 1 ou -1 pour une limite a droite ou a gauche, limit(sin(x)/x,x=0) ou limit(abs(x)/x,x=0,1). Raccourci: SHIFT MIXEDFRAC", "sin(x)/x,x=0", "exp(-1/x),x=0,1", CAT_CATEGORY_CALCULUS},
  {"line_width_", "line_width_", "Prefixe d'epaisseur (2 a 8)", 0, 0, CAT_CATEGORY_PROGCMD},
  {"linear_regression(Xlist,Ylist)", 0, "Regression lineaire.", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
  {"linear_regression_plot(Xlist,Ylist)", 0, "Graphe d'une regression lineaire.", "#X,Y:=[1,2,3,4,5],[0,1,3,4,4];linear_regression_plot(X,Y);scatterplot(X,Y)", 0, CAT_CATEGORY_STATS },
  {"linetan(expr,x,x0)", 0, "Tangente au graphe en x=x0.", "sin(x),x,pi/2", 0, CAT_CATEGORY_PLOT},
  {"linsolve([eq1,eq2,..],[x,y,..])", 0, "Resolution de systeme lineaire. Peut utiliser le resultat de lu pour resolution en O(n^2).","[x+y=1,x-y=2],[x,y]", "#p,l,u:=lu([[1,2],[3,4]]); linsolve(p,l,u,[5,6])", CAT_CATEGORY_SOLVE | (CAT_CATEGORY_LINALG <<8) | (CAT_CATEGORY_MATRIX << 16)},
  {"logarithmic_regression(Xlist,Ylist)", 0, "Regression logarithmique.", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
  {"lu(A)", 0, "decomposition LU de la matrice A, P*A=L*U, renvoie P permutation, L et U triangulaires inferieure et superieure", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
  {"magenta", "magenta", "Option d'affichage", "#display=magenta", 0, CAT_CATEGORY_PROGCMD},
  {"map(l,f)", 0, "Applique f aux elements de la liste l.","[1,2,3],x->x^2", 0, CAT_CATEGORY_LIST},
  {"matpow(A,n)", 0, "Renvoie A^n, la matrice A la puissance n", "[[1,2],[3,4]],n","#assume(n>=1);matpow([[0,2],[0,4]],n)", CAT_CATEGORY_MATRIX},
  {"matrix(l,c,func)", 0, "Matrice de terme general donne.", "2,3,(j,k)->j^k", 0, CAT_CATEGORY_MATRIX},
  {"mean(l)", 0, "Moyenne arithmetique liste l", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_STATS},
  {"median(l)", 0, "Mediane", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_STATS},
  {"montre_tortue ", "montre_tortue ", "Affiche la tortue", 0, 0, CAT_CATEGORY_LOGO},
  {"mult_c_conjugate", 0, "Multiplier par le conjugue complexe.", "1+2*i", 0,  (CAT_CATEGORY_COMPLEXNUM << 8)},
  {"mult_conjugate", 0, "Multiplier par le conjugue (sqrt).", "sqrt(2)-sqrt(3)", 0, CAT_CATEGORY_ALGEBRA},
  {"normald([mu,sigma],x)", 0, "Loi normale, par defaut mu=0 et sigma=1. normald_cdf([mu,sigma],x) probabilite que \"loi normale <=x\" par ex. normald_cdf(1.96). normald_icdf([mu,sigma],t) renvoie x tel que \"loi normale <=x\" vaut t, par ex. normald_icdf(0.975) ", "1.2", 0, CAT_CATEGORY_PROBA},
  {"not(x)", 0, "Non logique.", 0, 0, CAT_CATEGORY_PROGCMD},
  {"numer(x)", 0, "Numerateur de x.", "3/4", 0, CAT_CATEGORY_POLYNOMIAL},
  {"odesolve(f(t,y),[t,y],[t0,y0],t1)", 0, "Solution approchee d'equation differentielle y'=f(t,y) et y(t0)=y0, valeur en t1 (ajouter curve pour les valeurs intermediaires de y)", "sin(t*y),[t,y],[0,1],2", "0..pi,(t,v)->{[-v[1],v[0]]},[0,1]", CAT_CATEGORY_SOLVE},
  {"partfrac(p,x)", 0, "Decomposition en elements simples. Raccourci p=>+", "1/(x^4-1)", 0, CAT_CATEGORY_ALGEBRA},
  {"pas_de_cote n", "pas_de_cote ", "Saut lateral de la tortue, par defaut n=10", "#pas_de_cote 30", 0, CAT_CATEGORY_LOGO},
  {"plot(expr,x)", 0, "Graphe de fonction. Par exemple plot(sin(x)), plot(ln(x),x.0,5)", "ln(x),x=0..5", "1/x,x=1..5,xstep=1", CAT_CATEGORY_PLOT},
#ifdef RELEASE
  {"plotarea(expr,x=a..b,[n,meth])", 0, "Aire sous la courbe selon une methode d'integration.", "1/x,x=1..3,2,trapeze", 0, CAT_CATEGORY_PLOT},
#endif
  {"plotparam([x,y],t)", 0, "Graphe en parametriques. Par exemple plotparam([sin(3t),cos(2t)],t,0,pi) ou plotparam(exp(i*t),t,0,pi)", "[sin(3t),cos(2t)],t,0,pi", "[t^2,t^3],t=-1..1,tstep=0.1", CAT_CATEGORY_PLOT},
  {"plotpolar(r,theta)", 0, "Graphe en polaire.","cos(3*x),x,0,pi", "1/(1+cos(x)),x=0..pi,xstep=0.05", CAT_CATEGORY_PLOT},
  {"plotcontour(expr,[x=xm..xM,y=ym..yM],niveaux)", 0, "Lignes de niveau de expr.", "x^2+2y^2, [x=-2..2,y=-2..2],[1,2]", 0, CAT_CATEGORY_PLOT},
  {"plotfield(f(t,y), [t=tmin..tmax,y=ymin..ymax])", 0, "Champ des tangentes de y'=f(t,y), optionnellement graphe avec plotode=[t0,y0]", "sin(t*y), [t=-3..3,y=-3..3],plotode=[0,1]", "5*[-y,x], [x=-1..1,y=-1..1]", CAT_CATEGORY_PLOT},
  {"plotode(f(t,y), [t=tmin..tmax,y],[t0,y0])", 0, "Graphe de solution d'equation differentielle y'=f(t,y), y(t0)=y0.", "sin(t*y),[t=-3..3,y],[0,1]", 0, CAT_CATEGORY_PLOT},
  {"plotlist(list)", 0, "Graphe d'une liste", "[3/2,2,1,1/2,3,2,3/2]", "[1,13],[2,10],[3,15],[4,16]", CAT_CATEGORY_PLOT},
  {"plotseq(f(x),x=[u0,m,M],n)", 0, "Trace f(x) sur [m,M] et n termes de la suite recurrente u_{n+1}=f(u_n) de 1er terme u0.","sqrt(2+x),x=[6,0,7],5", 0, CAT_CATEGORY_PLOT},
  {"plus_point", "plus_point", "Option d'affichage", "#display=blue+plus_point", 0, CAT_CATEGORY_PROGCMD },
  {"point(x,y)", 0, "Point", "1,2", 0, CAT_CATEGORY_PROGCMD},
  {"polygon(list)", 0, "Polygone ferme.", "1-i,2+i,3", 0, CAT_CATEGORY_PROGCMD},
  {"polygonscatterplot(Xlist,Ylist)", 0, "Nuage de points relies.", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
  {"polynomial_regression(Xlist,Ylist,n)", 0, "Regression polynomiale de degre <= n.", "[1,2,3,4,5],[0,1,3,4,4],2", 0, CAT_CATEGORY_STATS},
  {"pour (boucle Xcas)", "pour  de  jusque  faire  fpour;", "Boucle definie.","#pour j de 1 jusque 10 faire print(j,j^2); fpour;", 0, CAT_CATEGORY_PROG},
  {"power_regression(Xlist,Ylist,n)", 0, "Regression puissance.", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
  {"powmod(a,n,p[,P,x])", 0, "Renvoie a^n mod p, ou a^n mod un entier p et un polynome P en x.","123,456,789", "x+1,452,19,x^4+x+1,x", CAT_CATEGORY_ARIT},
  {"print(expr)", 0, "Afficher dans la console", 0, 0, CAT_CATEGORY_PROG},
  {"proot(p)", 0, "Racines reelles et complexes approchees d'un polynome. Exemple proot([1,2.1,3,4.2]) ou proot(x^3+2.1*x^2+3x+4.2)", "x^3+2.1*x^2+3x+4.2", 0, CAT_CATEGORY_POLYNOMIAL},
  {"purge(x)", 0, "Purge le contenu de la variable x. Raccourci SHIFT-FORMAT", 0, 0, CAT_CATEGORY_PROGCMD | (CAT_CATEGORY_SOFUS<<8)},
  {"python(f)", 0, "Affiche la fonction f en syntaxe Python.", 0, 0, CAT_CATEGORY_PROGCMD},
  {"python_compat(0|1|2)", 0, "python_compat(0) syntaxe Xcas, python_compat(1) syntaxe Python avec ^ interprete comme puissance, python_compat(2) ^ interprete comme ou exclusif bit a bit", "0", "1", CAT_CATEGORY_PROG},
  {"qr(A)", 0, "Factorisation A=Q*R avec Q orthogonale et R triangulaire superieure", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
  {"quartile1(l)", 0, "1er quartile", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_STATS},
  {"quartile3(l)", 0, "3eme quartile", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_STATS},
  {"quo(p,q,x)", 0, "Quotient de division euclidienne polynomiale en x.", 0, 0, CAT_CATEGORY_POLYNOMIAL},
  {"quote(x)", 0, "Renvoie l'expression x non evaluee.", 0, 0, CAT_CATEGORY_ALGEBRA},
  {"rand()", "rand()", "Reel aleatoire entre 0 et 1", 0, 0, CAT_CATEGORY_PROBA},
  {"randint(n)", 0, "Entier aleatoire entre 1 et n ou entre a et b si on fournit 2 arguments", "6", "5,20", CAT_CATEGORY_PROBA},
  {"ranm(n,m,[loi,parametres])", 0, "Matrice aleatoire a coefficients entiers ou selon une loi de probabilites (ranv pour un vecteur). Exemples ranm(2,3), ranm(3,2,binomial,20,.3), ranm(4,2,normald,0,1)", "4,2,normald,0,1", "3,3,10", CAT_CATEGORY_MATRIX},
  {"ranv(n,[loi,parametres])", 0, "Vecteur aleatoire", "4,normald,0,1", "10,30", CAT_CATEGORY_LINALG},
  {"ratnormal(x)", 0, "Ecrit sous forme d'une fraction irreductible.", "(x+1)/(x^2-1)^2", 0, CAT_CATEGORY_ALGEBRA},
  {"re(z)", 0, "Partie reelle.", "1+i", 0, CAT_CATEGORY_COMPLEXNUM},
  {"read(\"filename\")", "read(\"", "Lire un fichier. Voir aussi write", 0, 0, CAT_CATEGORY_PROGCMD},
  {"rectangle_plein a,b", "rectangle_plein ", "Rectangle direct rempli depuis la tortue de cotes a et b (si b est omis, la tortue remplit un carre)", "#rectangle_plein 30", "#rectangle_plein(20,40)", CAT_CATEGORY_LOGO},
  {"recule n", "recule ", "La tortue recule de n pas, par defaut n=10", "#recule 30", 0, CAT_CATEGORY_LOGO},
  {"red", "red", "Option d'affichage", "#display=red", 0, CAT_CATEGORY_PROGCMD},
  {"rem(p,q,x)", 0, "Reste de division euclidienne polynomiale en x", 0, 0, CAT_CATEGORY_POLYNOMIAL},
  {"repete(n,...)", "repete( ", "Repete plusieurs fois les instructions", "#repete(4,avance,tourne_gauche)", 0, CAT_CATEGORY_LOGO},
#ifdef RELEASE
  {"residue(f(z),z,z0)", 0, "Residu de l'expression en z0.", "1/(x^2+1),x,i", 0, CAT_CATEGORY_COMPLEXNUM},
#endif
  {"resultant(p,q,x)", 0, "Resultant en x des polynomes p et q.", "#P:=x^3+p*x+q;resultant(P,P',x);", 0, CAT_CATEGORY_POLYNOMIAL},
  {"revert(p[,x])", 0, "Developpement de Taylor reciproque, p doit etre nul en 0","x+x^2+x^4", 0, CAT_CATEGORY_CALCULUS},
  {"rgb(r,g,b)", 0, "couleur definie par niveau de rouge, vert, bleu entre 0 et 255", "255,0,255", 0, CAT_CATEGORY_PROGCMD},
  {"rhombus_point", "rhombus_point", "Option d'affichage", "#display=magenta+rhombus_point", 0, CAT_CATEGORY_PROGCMD },
  {"rond n", "rond ", "Cercle tangent a la tortue de rayon n. Utiliser rond n,theta pour un arc de cercle.", "#rond 30", "#rond(30,90)", CAT_CATEGORY_LOGO},
  {"rsolve(equation,u(n),[init])", 0, "Expression d'une suite donnee par une recurrence.","u(n+1)=2*u(n)+3,u(n),u(0)=1", "([u(n+1)=3*v(n)+u(n),v(n+1)=v(n)+u(n)],[u(n),v(n)],[u(0)=1,v(0)=2]", CAT_CATEGORY_SOLVE},
  {"saute n", "saute ", "La tortue fait un saut de n pas, par defaut n=10", "#saute 30", 0, CAT_CATEGORY_LOGO},
  {"scatterplot(Xlist,Ylist)", 0, "Nuage de points.", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
  {"segment(A,B)", 0, "Segment", "1,2+i", 0, CAT_CATEGORY_PROGCMD},
  {"seq(expr,var,a,b[,pas])", 0, "Liste de terme general donne.","j^2,j,1,10", "j^2,j,1,10,2", CAT_CATEGORY_LIST},
  {"si (test Xcas)", "si  alors  sinon  fsi;", "Test.", "#f(x):=si x>0 alors x; sinon -x; fsi;// valeur absolue", 0, CAT_CATEGORY_PROG},
  {"sign(x)", 0, "Renvoie -1 si x est negatif, 0 si x est nul et 1 si x est positif.", 0, 0, CAT_CATEGORY_REAL},
  {"simplify(expr)", 0, "Renvoie en general expr sous forme simplifiee. Raccourci expr=>/", "sin(3x)/sin(x)", "ln(4)-ln(2)", CAT_CATEGORY_ALGEBRA},
  {"solve(equation,x)", 0, "Resolution exacte d'une equation en x (ou d'un systeme polynomial). Utiliser csolve pour les solutions complexes, linsolve pour un systeme lineaire. Raccourci SHIFT XthetaT", "x^2-x-1=0,x", "[x^2-y^2=0,x^2-z^2=0],[x,y,z]", CAT_CATEGORY_SOLVE},
  {"sort(l)", 0, "Trie une liste.","[3/2,2,1,1/2,3,2,3/2]", "[[1,2],[2,3],[4,3]],(x,y)->when(x[1]==y[1],x[0]>y[0],x[1]>y[1]", CAT_CATEGORY_LIST},
  {"square_point", "square_point", "Option d'affichage", "#display=cyan+square_point", 0, CAT_CATEGORY_PROGCMD },
  {"star_point", "star_point", "Option d'affichage", "#display=magenta+star_point", 0, CAT_CATEGORY_PROGCMD },
  {"stddev(l)", 0, "Ecart-type d'une liste l", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_STATS},
  {"subst(a,b=c)", 0, "Remplace b par c dans a. Raccourci a(b=c). Pour faire plusieurs remplacements, saisir subst(expr,[b1,b2...],[c1,c2...])", "x^2,x=3", "x+y^2,[x,y],[1,2]", CAT_CATEGORY_ALGEBRA},
  {"sum(f,k,m,M)", 0, "Somme de l'expression f dependant de k pour k variant de m a M. Exemple sum(k^2,k,1,n)=>*. Raccourci ALPHA F3", "k,k,1,n", "k^2,k", CAT_CATEGORY_CALCULUS},
  {"svd(A)", 0, "Singular Value Decomposition, renvoie U orthogonale, S vecteur des valeurs singulières, Q orthogonale tels que A=U*diag(S)*tran(Q).", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
  {"tabvar(f,[x=a..b])", 0, "Tableau de variations de l'expression f, avec arguments optionnels la variable x dans l'intervalle a..b.", "sqrt(x^2+x+1)", "[cos(t),sin(3t)],t", CAT_CATEGORY_CALCULUS},
  {"tantque (boucle Xcas)", "tantque  faire  ftantque;", "Boucle indefinie.", "#j:=13; tantque j!=1 faire j:=when(even(j),j/2,3j+1); print(j); ftantque;", 0, CAT_CATEGORY_PROG},
  {"taylor(f,x=a,n,[polynom])", 0, "Developpement de Taylor de l'expression f en x=a a l'ordre n, ajouter le parametre polynom pour enlever le terme de reste.","sin(x),x=0,5", "sin(x),x=0,5,polynom", CAT_CATEGORY_CALCULUS},
  {"tchebyshev1(n)", 0, "Polynome de Tchebyshev de 1ere espece: cos(n*x)=T_n(cos(x))", "10", 0, CAT_CATEGORY_POLYNOMIAL},
  {"tchebyshev2(n)", 0, "Polynome de Tchebyshev de 2eme espece: sin((n+1)*x)=sin(x)*U_n(cos(x))", "10", 0, CAT_CATEGORY_POLYNOMIAL},
  {"tcollect(expr)", 0, "Linearisation trigonometrique et regroupement.","sin(x)+cos(x)", 0, CAT_CATEGORY_TRIG},
  {"texpand(expr)", 0, "Developpe les fonctions trigonometriques, exp et ln.","sin(3x)", "ln(x*y)", CAT_CATEGORY_TRIG},
  {"time(cmd)", 0, "Temps pour effectuer une commande ou mise a l'heure de horloge","int(1/(x^4+1),x)","8,0", CAT_CATEGORY_PROG},
  {"tlin(expr)", 0, "Linearisation trigonometrique de l'expression.","sin(x)^3", 0, CAT_CATEGORY_TRIG},
  {"tourne_droite n", "tourne_droite ", "La tortue tourne de n degres, par defaut n=90", "#tourne_droite 45", 0, CAT_CATEGORY_LOGO},
  {"tourne_gauche n", "tourne_gauche ", "La tortue tourne de n degres, par defaut n=90", "#tourne_gauche 45", 0, CAT_CATEGORY_LOGO},
  {"tran(A)", 0, "Transposee de la matrice A. Pour la transconjuguee utiliser trn(A) ou A^*.", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
  {"triangle_point", "triangle_point", "Option d'affichage", "#display=yellow+triangle_point", 0, CAT_CATEGORY_PROGCMD},
  {"trig2exp(expr)", 0, "Convertit les fonctions trigonometriques en exponentielles.","cos(x)^3", 0, CAT_CATEGORY_TRIG},
  {"trigcos(expr)", 0, "Exprime sin^2 et tan^2 avec cos^2.","sin(x)^4", 0, CAT_CATEGORY_TRIG},
  {"trigsin(expr)", 0, "Exprime cos^2 et tan^2 avec sin^2.","cos(x)^4", 0, CAT_CATEGORY_TRIG},
  {"trigtan(expr)", 0, "Exprime cos^2 et sin^2 avec tan^2.","cos(x)^4", 0, CAT_CATEGORY_TRIG},
  {"uniformd(a,b,x)", 0, "loi uniforme sur [a,b] de densite 1/(b-a)", 0, 0, CAT_CATEGORY_PROBA},
  {"v augmente_de n", " augmente_de ", "La variable v augmente de n, ou de n %", "#v:=3; v augmente_de 1", 0, CAT_CATEGORY_SOFUS},
  {"v diminue_de n", " diminue_de ", "La variable v diminue de n, ou de n %", "#v:=3; v diminue_de 1", 0, CAT_CATEGORY_SOFUS},
  {"v est_divise_par n", " est_divise_par ", "La variable v est divisee par n", "#v:=3; v est_divise_par 2", 0, CAT_CATEGORY_SOFUS},
  {"v est_eleve_puissance n", " est_eleve_puissance ", "La variable v est eleveee a la puissance n", "#v:=3; v est_eleve_puissance 2", 0, CAT_CATEGORY_SOFUS},
  {"v est_multiplie_par n", " est_multiplie_par ", "La variable v est multipliee par n", "#v:=3; v est_multiplie_par 2", 0, CAT_CATEGORY_SOFUS},
  //{"version", "version()", "Khicas 1.5.0, (c) B. Parisse et al. www-fourier.ujf-grenoble.fr/~parisse. License GPL version 2. Interface adaptee d'Eigenmath pour Casio, G. Maia, http://gbl08ma.com", 0, 0, CAT_CATEGORY_PROGCMD},
  {"write(\"filename\",var)", "write(\"", "Sauvegarde une ou plusieurs variables dans un fichier. Par exemple f(x):=x^2; write(\"func_f\",f).",  0, 0, CAT_CATEGORY_PROGCMD},
  {"yellow", "yellow", "Option d'affichage", "#display=yellow", 0, CAT_CATEGORY_PROGCMD},
  {"|", "|", "Ou logique", "#1|2", 0, CAT_CATEGORY_PROGCMD},
  {"~", "~", "Complement", "#~7", 0, CAT_CATEGORY_PROGCMD},
};

const char aide_khicas_string[]="Aide Khicas";
const char shortcuts_string[]="Pour mettre a l'heure l'horloge, tapez heure,minute puis touche STO puis , par exemple 13,10=>,\n\nRaccourcis clavier (shell et editeur)\nF1-F3: selon legendes\nF4: catalogue\nF5: blocage en minuscule ou bascule minuscule/majuscule\nF6: menu fichier, configuration\nshift-PRGM: caracteres pour programmer et commande debug\nshift-FRAC: graphiques plot...\n\nOPTN: options\nshift-QUIT: tortue\nshift-Lst: listes\nshift-Mtr: matrices\nVARS: liste des variables (shell) ou dessin tortue (editeur)\n=>+: partfrac\n=>*: factor\n=>sin/cos/tan\n=>=>: solve\nHistorique calculs:\nF3 Editeur 2d ou graphique ou texte selon objet\nshift-F3: editeur texte\n+ ou - modifie un parametre\n\nEditeur d'expressions\npave directionnel: deplace la selection dans l'arborescence de l'expression\nshift-droit/gauche echange selection avec argument a droite ou a gauche\nALPHA-droit/gauche dans une somme ou un produit: augmente la selection avec argument droit ou gauche\nF3: Editer selection, shift-F3: taille police + grande, ALPHA-F3: taille plus petite\nF4: catalogue\nF5: minuscule/majuscule\nF6: Evaluer la selection, shift-F6: valeur approchee, ALPHA-F6: commande regroup\nDEL: supprime l'operateur racine de la selection\n\nEditeur de scripts\nshift-CLIP: marque le debut de la selection, deplacer le curseur vers la fin puis DEL pour effacer ou shift-CLIP pour copier sans effacer. shift-PASTE pour coller.\nF6-6 recherche seule: entrer un mot puis EXE puis EXIT. Taper EXE pour l'occurence suivante, AC pour annuler.\nF6-6 remplacer: entrer un mot puis EXE puis le remplacement et EXE. Taper EXE ou EXIT pour remplacer ou non et passer a l'occurence suivante, AC pour annuler\nshift-Ans: tester syntaxe\n\nRaccourcis Graphes:\n+ - zoom\n(-): zoomout selon y\n*: autoscale\n/: orthonormalisation\nOPTN: axes on/off";
const char apropos_string[]="Khicas 1.5.0, (c) 2019 B. Parisse et al. www-fourier.univ-grenoble-alpes.fr/~parisse.\nLicense GPL version 2.\nInterface adaptee d'Eigenmath pour Casio, G. Maia (http://gbl08ma.com), Mike Smith, Nemhardy, LePhenixNoir, ...";

int CAT_COMPLETE_COUNT=sizeof(completeCat)/sizeof(catalogFunc);

std::string insert_string(int index){
  std::string s;
  if (completeCat[index].insert)
    s=completeCat[index].insert;
  else {
    s=completeCat[index].name;
    int pos=s.find('(');
    if (pos>=0 && pos<s.size())
      s=s.substr(0,pos+1);
  }
  return s;//s+' ';
}

int showCatalog(char* insertText,int preselect,int menupos) {
  // returns 0 on failure (user exit) and 1 on success (user chose a option)
  MenuItem menuitems[CAT_CATEGORY_LOGO+1];
  menuitems[CAT_CATEGORY_ALL].text = (char*)"Tout";
  menuitems[CAT_CATEGORY_ALGEBRA].text = (char*)"Algebre";
  menuitems[CAT_CATEGORY_LINALG].text = (char*)"Algebre lineaire";
  menuitems[CAT_CATEGORY_CALCULUS].text = (char*)"Analyse";
  menuitems[CAT_CATEGORY_ARIT].text = (char*)"Arithmetic, crypto";
  menuitems[CAT_CATEGORY_COMPLEXNUM].text = (char*)"Complexes";
  menuitems[CAT_CATEGORY_PLOT].text = (char*)"Graphes";
  menuitems[CAT_CATEGORY_POLYNOMIAL].text = (char*)"Polynomes";
  menuitems[CAT_CATEGORY_PROBA].text = (char*)"Probabilites";
  menuitems[CAT_CATEGORY_PROG].text = (char*)"Programmes PRGM";
  menuitems[CAT_CATEGORY_PROGCMD].text = (char*)"Programmes cmds";
  menuitems[CAT_CATEGORY_REAL].text = (char*)"Reels";
  menuitems[CAT_CATEGORY_SOLVE].text = (char*)"Resoudre";
  menuitems[CAT_CATEGORY_STATS].text = (char*)"Statistiques";
  menuitems[CAT_CATEGORY_TRIG].text = (char*)"Trigonometrie";
  menuitems[CAT_CATEGORY_OPTIONS].text = (char*)"Options";
  menuitems[CAT_CATEGORY_LIST].text = (char*)"Listes";
  menuitems[CAT_CATEGORY_MATRIX].text = (char*)"Matrices";
  menuitems[CAT_CATEGORY_SOFUS].text = (char*)"Modifier des variables";
  menuitems[CAT_CATEGORY_LOGO].text = (char*)"Tortue shift QUIT";
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=sizeof(menuitems)/sizeof(MenuItem);
  menu.scrollout=1;
  menu.title = (char*)"Liste de commandes";
  //puts("catalog 1");
  while(1) {
    if (preselect)
      menu.selection=preselect;
    else {
      if (menupos>0)
	menu.selection=menupos;
      int sres = doMenu(&menu);
      if (sres != MENU_RETURN_SELECTION && sres!=KEY_CTRL_EXE)
	return 0;
    }
    // puts("catalog 3");
    if(doCatalogMenu(insertText, menuitems[menu.selection-1].text, menu.selection-1)) 
      return 1;
    if (preselect)
      return 0;
  }
  return 0;
}

  int showCatalog(char * text,int nmenu){
    return showCatalog(text,0,nmenu);
  }

  bool isalphanum(char c){
    return (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9');
  }

// 0 on exit, 1 on success
int doCatalogMenu(char* insertText, const char* title, int category) {
  for (;;){
    int allcmds=builtin_lexer_functions_end()-builtin_lexer_functions_begin();
    int allopts=lexer_tab_int_values_end-lexer_tab_int_values_begin;
    bool isall=category==CAT_CATEGORY_ALL;
    bool isopt=category==CAT_CATEGORY_OPTIONS;
    int nitems = isall? allcmds:(isopt?allopts:CAT_COMPLETE_COUNT);
    MenuItem menuitems[nitems];
    int cur = 0,curmi = 0,i=0;
    gen g;
    while(cur<nitems) {
      menuitems[curmi].type = MENUITEM_NORMAL;
      menuitems[curmi].color = _BLACK;    
      if (isall || isopt) {
	const char * text=isall?(builtin_lexer_functions_begin()+curmi)->first:(lexer_tab_int_values_begin+curmi)->keyword;
	menuitems[curmi].text = (char*) text;
	menuitems[curmi].isfolder = allcmds; // assumes allcmds>allopts
	menuitems[curmi].token=isall?((builtin_lexer_functions_begin()+curmi)->second.subtype+256):((lexer_tab_int_values_begin+curmi)->subtype+(lexer_tab_int_values_begin+curmi)->return_value*256);
	// menuitems[curmi].token=isall?find_or_make_symbol(text,g,0,false,contextptr):((lexer_tab_int_values_begin+curmi)->subtype+(lexer_tab_int_values_begin+curmi)->return_value*256);
	for (;i<CAT_COMPLETE_COUNT;++i){
	  const char * catname=completeCat[i].name;
	  int tmp=strcmp(catname,text);
	  if (tmp>=0){
	    size_t st=strlen(text),j=tmp?0:st;
	    for (;j<st;++j){
	      if (catname[j]!=text[j])
		break;
	    }
	    if (j==st && (!isalphanum(catname[j]))){
	      menuitems[curmi].isfolder = i;
	      ++i;
	    }
	    break;
	  }
	}
	// compare text with completeCat
	++curmi;
      }
      else {
	int cat=completeCat[cur].category;
	if ( (cat & 0xff) == category ||
	     (cat & 0xff00) == (category<<8) ||
	     (cat & 0xff0000) == (category <<16)
	     ){
	  menuitems[curmi].isfolder = cur; // little hack: store index of the command in the full list in the isfolder property (unused by the menu system in this case)
	  menuitems[curmi].text = (char *) completeCat[cur].name;
	  curmi++;
	}
      }
      cur++;
    }
  
    Menu menu;
    menu.items=menuitems;
    menu.numitems=curmi;
    if (isopt){ menu.selection=5; menu.scroll=4; }
    if (curmi>=100)
      SetSetupSetting( (unsigned int)0x14, 0x88);	
    // DisplayStatusArea();
    menu.scrollout=1;
    menu.title = (char *) title;
    menu.type = MENUTYPE_FKEYS;
    menu.height = 12;
    while(1) {
      drawRectangle(0,56,LCD_WIDTH_PX,8,COLOR_BLACK);
      // PrintMini(0,57,(category==CAT_CATEGORY_ALL?"input| ex1 | ex2 |     |    | help":"input| ex1 | ex2 |cmds |    |help"),MINI_REV);
      int sres = doMenu(&menu);
      if (sres==KEY_CTRL_F4 && category!=CAT_CATEGORY_ALL){
	break;
      }
      if(sres == MENU_RETURN_EXIT){
	reset_alpha();
	return sres;
      }
      int index=menuitems[menu.selection-1].isfolder;
      if(sres == KEY_CTRL_CATALOG) {
	const char * example=index<allcmds?completeCat[index].example:0;
	const char * example2=index<allcmds?completeCat[index].example2:0;
#if 0
	textArea text;
	text.editable=false;
	text.clipline=-1;
	text.title = (char*)"Aide sur la commande";
	text.allowF1=true;
	text.python=python_compat(contextptr);
	std::vector<textElement> & elem=text.elements;
	elem = std::vector<textElement> (example2?4:3);
	elem[0].s = index<allcmds?completeCat[index].name:menuitems[menu.selection-1].text;
	elem[0].newLine = 0;
	//elem[0].color = COLOR_BLUE;
	elem[1].newLine = 1;
	elem[1].lineSpacing = 1;
	elem[1].minimini=1;
	std::string autoexample;
	if (index<allcmds)
	  elem[1].s = completeCat[index].desc;
	else {
	  int token=menuitems[menu.selection-1].token;
	  elem[1].s="Desole, pas d'aide disponible...";
	  // *logptr(contextptr) << token << endl;
	  if (isopt){
	    if (token==_INT_PLOT+T_NUMBER*256){
	      autoexample="display="+elem[0].s;
	      elem[1].s ="Option d'affichage: "+ autoexample;
	    }
	    if (token==_INT_COLOR+T_NUMBER*256){
	      autoexample="display="+elem[0].s;
	      elem[1].s="Option de couleur: "+ autoexample;
	    }
	    if (token==_INT_SOLVER+T_NUMBER*256){
	      autoexample=elem[0].s;
	      elem[1].s="Option de fsolve: " + autoexample;
	    }
	    if (token==_INT_TYPE+T_TYPE_ID*256){
	      autoexample=elem[0].s;
	      elem[1].s="Type d'objet: " + autoexample;
	    }
	  }
	  if (isall){
	    if (token==T_UNARY_OP || token==T_UNARY_OP_38)
	      elem[1].s=elem[0].s+"(args)";
	  }
	}
	std::string ex("F2: ");
	elem[2].newLine = 1;
	elem[2].lineSpacing = 0;
	//elem[2].minimini=1;
	if (example){
	  if (example[0]=='#')
	    ex += example+1;
	  else {
	    ex += insert_string(index);
	    ex += example;
	    ex += ")";
	  }
	  elem[2].s = ex;
	  if (example2){
	    string ex2="F3: ";
	    if (example2[0]=='#')
	      ex2 += example2+1;
	    else {
	      ex2 += insert_string(index);
	      ex2 += example2;
	      ex2 += ")";
	    }
	    elem[3].newLine = 1;
	    // elem[3].lineSpacing = 0;
	    //elem[3].minimini=1;
	    elem[3].s=ex2;
	  }
	}
	else {
	  if (autoexample.size())
	    elem[2].s=ex+autoexample;
	  else
	    elem.pop_back();
	}
	sres=doTextArea(&text);
#else
	string cmdname = index<allcmds?completeCat[index].name:menuitems[menu.selection-1].text;
	string desc;
	std::string autoexample;
	if (index<allcmds)
	  desc = completeCat[index].desc;
	else {
	  int token=menuitems[menu.selection-1].token;
	  desc ="Desole, pas d'aide disponible...";
	  if (isopt){
	    if (token==_INT_PLOT+T_NUMBER*256){
	      autoexample="display="+cmdname;
	      desc ="Option d'affichage: "+ autoexample;
	    }
	    if (token==_INT_COLOR+T_NUMBER*256){
	      autoexample="display="+cmdname;
	      desc ="Option de couleur: "+ autoexample;
	    }
	    if (token==_INT_SOLVER+T_NUMBER*256){
	      autoexample=cmdname;
	      desc ="Option de fsolve: " + autoexample;
	    }
	    if (token==_INT_TYPE+T_TYPE_ID*256){
	      autoexample=cmdname;
	      desc ="Type d'objet: " + autoexample;
	    }
	  }
	  if (isall){
	    if (token==T_UNARY_OP || token==T_UNARY_OP_38)
	      desc =cmdname+"(args)";
	  }
	}
	std::string ex("Ans: "),ex2;
	if (example){
	  if (example[0]=='#')
	    ex += example+1;
	  else {
	    ex += insert_string(index);
	    ex += example;
	    ex += ")";
	  }
	  if (example2){
	    ex2="EXE: ";
	    if (example2[0]=='#')
	      ex2 += example2+1;
	    else {
	      ex2 += insert_string(index);
	      ex2 += example2;
	      ex2 += ")";
	    }
	  }
	}
	else {
	  if (autoexample.size())
	    ex2=ex+autoexample;
	}
	// cmdname, desc, ex1, ex2
	drawRectangle(0,0,320,222,_WHITE);
	numworks_draw_string(0,0,cmdname.c_str());
	vector<int> endlines;
	string res=cut_string(desc,40,endlines);
	if (!endlines.empty())
	  numworks_draw_string_small(0,20,res.substr(0,endlines[0]).c_str());
	for (int i=1;i<endlines.size();++i){
	  numworks_draw_string_small(0,20+18*i,res.substr(endlines[i-1]+5,endlines[i]-endlines[i-1]-5).c_str());
	}
	numworks_draw_string(0,20+18*endlines.size(),ex.c_str());
	numworks_draw_string(0,40+18*endlines.size(),ex2.c_str());
	while (1){
	  int key;
	  GetKey(&key);
	  if (key==KEY_CHAR_ANS || key==KEY_CTRL_EXE){
	    sres=key;
	    break;
	  }
	  if (key==KEY_CTRL_AC || key==KEY_CTRL_EXIT || key==KEY_CTRL_OK)
	    break;
	}
#endif
      }
      if (sres == KEY_CHAR_ANS || sres==KEY_CTRL_EXE) {
	reset_alpha();
	if (index<allcmds && completeCat[index].example){
	  std::string s(insert_string(index));
	  const char * example=0;
	  if (sres==KEY_CHAR_ANS)
	    example=completeCat[index].example;
	  else
	    example=completeCat[index].example2;
	  if (example){
	    if (example[0]=='#')
	      s=example+1;
	    else {
	      s += example;
	      s += ")";
	    }
	  }
	  strcpy(insertText, s.c_str());
	  return 1;
	}
	if (isopt){
	  int token=menuitems[menu.selection-1].token;
	  if (token==_INT_PLOT+T_NUMBER*256 || token==_INT_COLOR+T_NUMBER*256)
	    strcpy(insertText,"display=");
	  else
	    *insertText=0;
	  strcat(insertText,menuitems[menu.selection-1].text);
	  return 1;
	}
	sres=KEY_CTRL_F1;
      }
      if(sres == MENU_RETURN_SELECTION || sres == KEY_CTRL_OK) {
	reset_alpha();
	strcpy(insertText,index<allcmds?insert_string(index).c_str():menuitems[menu.selection-1].text);
	return 1;
      }
    }
    title="CATALOG";
    category=0;
  } // end endless for
}

  gen select_var(GIAC_CONTEXT){
    gen g(_VARS(0,contextptr));
    if (g.type!=_VECT || g._VECTptr->empty())
      return undef;
    vecteur & v=*g._VECTptr;
    MenuItem smallmenuitems[v.size()+3];
    vector<std::string> vs(v.size()+1);
    int i,total=0;
    const char typ[]="idzDcpiveSfEsFRmuMwgPF";
    for (i=0;i<v.size();++i){
      vs[i]=v[i].print(contextptr);
      if (v[i].type==giac::_IDNT){
	giac::gen w;
	v[i]._IDNTptr->in_eval(0,v[i],w,contextptr,true);
#if 1
	vector<int> vi(9);
	tailles(w,vi);
	total += vi[8];
	if (vi[8]<400)
	  vs[i]+=":="+w.print(contextptr);
	else {
	  vs[i] += " ~";
	  vs[i] += giac::print_INT_(vi[8]);
	  vs[i] += ',';
	  vs[i] += typ[w.type];
	}
#else
	if (taille(w,50)<50)
	  vs[i]+=": "+w.print(contextptr);
#endif
      }
      smallmenuitems[i].text=(char *) vs[i].c_str();
    }
    total +=
      // giac::syms().capacity()*(sizeof(string)+sizeof(giac::gen)+8)+sizeof(giac::sym_string_tab) +
      giac::turtle_stack().capacity()*sizeof(giac::logo_turtle) +
      // sizeof(giac::context)+contextptr->tabptr->capacity()*(sizeof(const char *)+sizeof(giac::gen)+8)+
      bytesize(giac::history_in(contextptr))+bytesize(giac::history_out(contextptr));
    vs[i]="purge(~"+giac::print_INT_(total)+')';
    smallmenuitems[i].text=(char *)vs[i].c_str();
    smallmenuitems[i+1].text=(char *)"assume(";
    smallmenuitems[i+2].text=(char *)"restart";
    Menu smallmenu;
    smallmenu.numitems=v.size()+3; 
    smallmenu.items=smallmenuitems;
    smallmenu.height=12;
    smallmenu.scrollbar=1;
    smallmenu.scrollout=1;
    smallmenu.title = (char*)"Variables";
    //MsgBoxPush(5);
    int sres = doMenu(&smallmenu);
    //MsgBoxPop();
    if (sres!=MENU_RETURN_SELECTION && sres!=KEY_CTRL_EXE)
      return undef;
    if (smallmenu.selection==1+v.size())
      return string2gen("purge(",false);
    if (smallmenu.selection==2+v.size())
      return string2gen("assume(",false);
    if (smallmenu.selection==3+v.size())
      return string2gen("restart",false);
    return v[smallmenu.selection-1];
  }
  
  const char * keytostring(int key,int keyflag,bool py,const giac::context * contextptr){
    const int textsize=512;
    static char text[textsize];
    if (key>=0x20 && key<=0x7e){
      text[0]=key;
      text[1]=0;
      return text;
    }
    switch (key){
    case KEY_CHAR_PLUS:
      return "+";
    case KEY_CHAR_MINUS:
      return "-";
    case KEY_CHAR_PMINUS:
      return "_";
    case KEY_CHAR_MULT:
      return "*";
    case KEY_CHAR_FRAC:
      return py?"\\":"solve(";
    case KEY_CHAR_DIV: 
      return "/";
    case KEY_CHAR_POW:
      return "^";
    case KEY_CHAR_ROOT:
      return "sqrt(";
    case KEY_CHAR_SQUARE:
      return py?"**2":"^2";
    case KEY_CHAR_CUBEROOT:
      return py?"**(1/3)":"^(1/3)";
    case KEY_CHAR_POWROOT:
      return py?"**(1/":"^(1/";
    case KEY_CHAR_RECIP:
      return py?"**-1":"^-1";
    case KEY_CHAR_THETA:
      return "arg(";
    case KEY_CHAR_VALR:
      return "abs(";
    case KEY_CHAR_ANGLE:
      return "polar_complex(";
    case KEY_CTRL_XTT:
      return xthetat?"t":"x";
    case KEY_CHAR_LN:
      return "ln(";
    case KEY_CHAR_LOG:
      return "log10(";
    case KEY_CHAR_EXPN10:
      return "10^";
    case KEY_CHAR_EXPN:
      return "exp(";
    case KEY_CHAR_SIN:
      return "sin(";
    case KEY_CHAR_COS:
      return "cos(";
    case KEY_CHAR_TAN:
      return "tan(";
    case KEY_CHAR_ASIN:
      return "asin(";
    case KEY_CHAR_ACOS:
      return "acos(";
    case KEY_CHAR_ATAN:
      return "atan(";
    case KEY_CTRL_MIXEDFRAC:
      return "limit(";
    case KEY_CTRL_FRACCNVRT:
      return "exact(";
      // case KEY_CTRL_FORMAT: return "purge(";
    case KEY_CTRL_FD:
      return "approx(";
    case KEY_CHAR_STORE:
      // if (keyflag==1) return "inf";
      return "=>";
    case KEY_CHAR_IMGNRY:
      return "i";
    case KEY_CHAR_PI:
      return "pi";
    case KEY_CTRL_VARS: {
      giac::gen var=select_var(contextptr);
      if (!giac::is_undef(var)){
	strcpy(text,(var.type==giac::_STRNG?*var._STRNGptr:var.print(contextptr)).c_str());
	return text;
      }
      return "VARS()";
    }
    case KEY_CHAR_EXP:
      return "e";
    case KEY_CHAR_ANS:
      return "ans()";
    case KEY_CTRL_INS:
      return ":=";
    case KEY_CHAR_MAT:{
      // const char * ptr=input_matrix(false); if (ptr) return ptr;
      if (showCatalog(text,17)) return text;
      return "";
    }
    case KEY_CHAR_LIST: {
      //const char * ptr=input_matrix(true); if (ptr) return ptr;
      if (showCatalog(text,16)) return text;
      return "";
    }
    case KEY_CTRL_PRGM:
      // open functions catalog, prgm submenu
      if(showCatalog(text,18))
	return text;
      return "";
    case KEY_CTRL_CATALOG:
      if(showCatalog(text,0)) 
	return text;
      return "";
    case KEY_CTRL_F4:
      if(showCatalog(text,0)) 
	return text;
      return "";
    case KEY_SHIFT_OPTN:
      if(showCatalog(text,10))
	return text;
      return "";
    case KEY_CTRL_OPTN:
      if(showCatalog(text,15))
	return text;
      return "";
    case KEY_CTRL_QUIT: 
      if(showCatalog(text,20))
	return text;
      return "";
    case KEY_CTRL_SETUP:
      if(showCatalog(text,7))
	return text;
      return "";
    case KEY_CTRL_PASTE:
      return paste_clipboard();
    case KEY_CHAR_DQUATE:
      return "\"";
    }
    return 0;
  }
  
  const char * keytostring(int key,int keyflag,GIAC_CONTEXT){
    return keytostring(key,keyflag,python_compat(contextptr),contextptr);
  }
  
  bool stringtodouble(const string & s1,double & d){
    gen g(s1,context0);
    g=evalf(g,1,context0);
    if (g.type!=_DOUBLE_){
      confirm("Invalid value",s1.c_str());
      return false;
    }
    d=g._DOUBLE_val;
    return true;
  }

  bool inputdouble(const char * msg1,double & d,GIAC_CONTEXT){
    string s1;
    inputline(msg1,(lang?"Nouvelle valeur?":"New value?"),s1,false,65,contextptr);
    return stringtodouble(s1,d);
  }
  
  int inputline(const char * msg1,const char * msg2,string & s,bool numeric,int ypos,GIAC_CONTEXT){
    // s="";
    int pos=s.size(),beg=0;
    for (;;){
      int X1=print_msg12(msg1,msg2,ypos-30);
      int textX=X1,textY=ypos;
      drawRectangle(textX,textY,LCD_WIDTH_PX-textX-4,18,COLOR_WHITE);
      if (pos-beg>36)
	beg=pos-12;
      if (int(s.size())-beg<36)
	beg=giac::giacmax(0,int(s.size())-36);
      textX=X1;
      numworks_draw_string(textX,textY,(s.substr(beg,pos-beg)+"|"+s.substr(pos,s.size()-pos)).c_str());
      // numworks_draw_string_small(textX,textY,s.substr(beg,pos-beg).c_str());// PrintMini(&textX,&textY,(unsigned char *)s.substr(beg,pos-beg).c_str(),0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      // numworks_draw_string_small(textX,textY,s.substr(pos,s.size()-pos).c_str());// PrintMini(&textX,&textY,(unsigned char*)s.substr(pos,s.size()-pos).c_str(),0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
      // int cursorpos=textX;
      // drawRectangle(cursorpos,textY+24,3,18,COLOR_BLACK); // cursor
      // PrintMini(0,58,"         |        |        |        |  A<>a  |       ",4);
      int key;
      GetKey(&key);
      // if (!giac::freeze) set_xcas_status();    
      if (key==KEY_CTRL_EXE || key==KEY_CTRL_OK)
	return KEY_CTRL_EXE;
      if (key>=32 && key<128){
	if (!numeric || key=='-' || (key>='0' && key<='9'))
	  s.insert(s.begin()+pos,char(key));
	++pos;
	continue;
      }
      if (key==KEY_CTRL_DEL){
	if (pos){
	  s.erase(s.begin()+pos-1);
	  --pos;
	}
	continue;
      }
      if (key==KEY_CTRL_AC){
	if (s=="")
	  return KEY_CTRL_EXIT;
	s="";
	pos=0;
	continue;
      }
      if (key==KEY_CTRL_EXIT)
	return key;
      if (key==KEY_CTRL_RIGHT){
	if (pos<s.size())
	  ++pos;
	continue;
      }
      if (key==KEY_SHIFT_RIGHT){
	pos=s.size();
	continue;
      }
      if (key==KEY_CTRL_LEFT){
	if (pos)
	  --pos;
	continue;
      }
      if (key==KEY_SHIFT_LEFT){
	pos=0;
	continue;
      }
      if (const char * ans=keytostring(key,0,false,contextptr)){
	insert(s,pos,ans);
	pos+=strlen(ans);
	continue;
      }
    }
  }

  void translate_fkey(int & key){
  }
  const char * console_menu(int key,int keyflag){
    return "factor";
  }

  logo_turtle * turtleptr=0;
  
  logo_turtle & turtle(){
    if (!turtleptr)
      turtleptr=new logo_turtle;
    return * turtleptr;
  }
  
  const int MAX_LOGO=368; // 512;

  std::vector<logo_turtle> & turtle_stack(){
    static std::vector<logo_turtle> * ans = 0;
    if (!ans){
      // initialize from python app storage
      ans=new std::vector<logo_turtle>(1,(*turtleptr));
     
    }
    return *ans;
  }

  logo_turtle vecteur2turtle(const vecteur & v){
    int s=int(v.size());
    if (s>=5 && v[0].type==_DOUBLE_ && v[1].type==_DOUBLE_ && v[2].type==_DOUBLE_ && v[3].type==_INT_ && v[4].type==_INT_ ){
      logo_turtle t;
      t.x=v[0]._DOUBLE_val;
      t.y=v[1]._DOUBLE_val;
      t.theta=v[2]._DOUBLE_val;
      int i=v[3].val;
      t.mark=(i%2)!=0;
      i=i >> 1;
      t.visible=(i%2)!=0;
      i=i >> 1;
      t.direct = (i%2)!=0;
      i=i >> 1;
      t.turtle_length = i & 0xff;
      i=i >> 8;
      t.color = i;
      t.radius = v[4].val;
      if (s>5 && v[5].type==_INT_)
	t.s=v[5].val;
      else
	t.s=-1;
      return t;
    }
#ifndef NO_STDEXCEPT
    setsizeerr(gettext("vecteur2turtle")); // FIXME
#endif
    return logo_turtle();
  }

  static int turtle_status(const logo_turtle & turtle){
    int status= (turtle.color << 11) | ( (turtle.turtle_length & 0xff) << 3) ;
    if (turtle.direct)
      status += 4;
    if (turtle.visible)
      status += 2;
    if (turtle.mark)
      status += 1;
    return status;
  }

  bool set_turtle_state(const vecteur & v,GIAC_CONTEXT){
    if (v.size()>=2 && v[0].type==_DOUBLE_ && v[1].type==_DOUBLE_){
      vecteur w(v);
      int s=int(w.size());
      if (s==2)
	w.push_back(double((*turtleptr).theta));
      if (s<4)
	w.push_back(turtle_status((*turtleptr)));
      if (s<5)
	w.push_back(0);
      if (w[2].type==_DOUBLE_ && w[3].type==_INT_ && w[4].type==_INT_){
	(*turtleptr)=vecteur2turtle(w);
#ifdef TURTLETAB
	turtle_stack_push_back(*turtleptr);
#else
	turtle_stack().push_back((*turtleptr));
#endif
	return true;
      }
    }
    return false;
  }

  gen turtle2gen(const logo_turtle & turtle){
    return gen(makevecteur(turtle.x,turtle.y,double(turtle.theta),turtle_status(turtle),turtle.radius,turtle.s),_LOGO__VECT);
  }

  gen turtle_state(GIAC_CONTEXT){
    return turtle2gen((*turtleptr));
  }

  static gen update_turtle_state(bool clrstring,GIAC_CONTEXT){
#ifdef TURTLETAB
    if (turtle_stack_size>=MAX_LOGO)
      return gensizeerr("Not enough memory");
#else
    if (turtle_stack().size()>=MAX_LOGO){
      ctrl_c=true; interrupted=true;
      return gensizeerr("Not enough memory");
    }
#endif
    if (clrstring)
      (*turtleptr).s=-1;
    (*turtleptr).theta = (*turtleptr).theta - floor((*turtleptr).theta/360)*360;
#ifdef TURTLETAB
    turtle_stack_push_back((*turtleptr));
#else
    turtle_stack().push_back((*turtleptr));
#endif
    gen res=turtle_state(contextptr);
#ifdef EMCC // should directly interact with canvas
    return gen(turtlevect2vecteur(turtle_stack()),_LOGO__VECT);
#endif
    return res;
  }

  gen _avance(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    // logo instruction
    double i;
    if (g.type!=_INT_){
      if (g.type==_VECT)
	i=(*turtleptr).turtle_length;
      else {
	gen g1=evalf_double(g,1,contextptr);
	if (g1.type==_DOUBLE_)
	  i=g1._DOUBLE_val;
	else
	  return gensizeerr(contextptr);
      }
    }
    else
      i=g.val;
    (*turtleptr).x += i * std::cos((*turtleptr).theta*deg2rad_d);
    (*turtleptr).y += i * std::sin((*turtleptr).theta*deg2rad_d) ;
    (*turtleptr).radius = 0;
    return update_turtle_state(true,contextptr);
  }
  static const char _avance_s []="avance";
  static define_unary_function_eval2 (__avance,&_avance,_avance_s,&printastifunction);
  define_unary_function_ptr5( at_avance ,alias_at_avance,&__avance,0,T_LOGO);

  static const char _forward_s []="forward";
  static define_unary_function_eval (__forward,&_avance,_forward_s);
  define_unary_function_ptr5( at_forward ,alias_at_forward,&__forward,0,true);

  gen _recule(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    // logo instruction
    if (g.type==_VECT)
      return _avance(-(*turtleptr).turtle_length,contextptr);
    return _avance(-g,contextptr);
  }
  static const char _recule_s []="recule";
  static define_unary_function_eval2 (__recule,&_recule,_recule_s,&printastifunction);
  define_unary_function_ptr5( at_recule ,alias_at_recule,&__recule,0,T_LOGO);

  static const char _backward_s []="backward";
  static define_unary_function_eval (__backward,&_recule,_backward_s);
  define_unary_function_ptr5( at_backward ,alias_at_backward,&__backward,0,true);

  gen _position(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    // logo instruction
    if (g.type!=_VECT)
      return makevecteur((*turtleptr).x,(*turtleptr).y);
    // return turtle_state();
    vecteur v = *g._VECTptr;
    int s=int(v.size());
    if (!s)
      return makevecteur((*turtleptr).x,(*turtleptr).y);
    v[0]=evalf_double(v[0],1,contextptr);
    if (s>1)
      v[1]=evalf_double(v[1],1,contextptr);
    if (s>2)
      v[2]=evalf_double(v[2],1,contextptr); 
    if (set_turtle_state(v,contextptr))
      return update_turtle_state(true,contextptr);
    return zero;
  }
  static const char _position_s []="position";
  static define_unary_function_eval2 (__position,&_position,_position_s,&printastifunction);
  define_unary_function_ptr5( at_position ,alias_at_position,&__position,0,T_LOGO);

  gen _cap(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    // logo instruction
    gen gg=evalf_double(g,1,contextptr);
    if (gg.type!=_DOUBLE_)
      return double((*turtleptr).theta);
    (*turtleptr).theta=gg._DOUBLE_val;
    (*turtleptr).radius = 0;
    return update_turtle_state(true,contextptr);
  }
  static const char _cap_s []="cap";
  static define_unary_function_eval2 (__cap,&_cap,_cap_s,&printastifunction);
  define_unary_function_ptr5( at_cap ,alias_at_cap,&__cap,0,T_LOGO);

  static const char _heading_s []="heading";
  static define_unary_function_eval (__heading,&_cap,_heading_s);
  define_unary_function_ptr5( at_heading ,alias_at_heading,&__heading,0,true);


  gen _tourne_droite(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    // logo instruction
    if (g.type!=_INT_){
      if (g.type==_VECT)
	(*turtleptr).theta -= 90;
      else {
	gen g1=evalf_double(g,1,contextptr);
	if (g1.type==_DOUBLE_)
	  (*turtleptr).theta -= g1._DOUBLE_val;
	else
	  return gensizeerr(contextptr);
      }
    }
    else
      (*turtleptr).theta -= g.val;
    (*turtleptr).radius = 0;
    return update_turtle_state(true,contextptr);
  }
  static const char _tourne_droite_s []="tourne_droite";
  static define_unary_function_eval2 (__tourne_droite,&_tourne_droite,_tourne_droite_s,&printastifunction);
  define_unary_function_ptr5( at_tourne_droite ,alias_at_tourne_droite,&__tourne_droite,0,T_LOGO);

  gen _tourne_gauche(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    // logo instruction
    if (g.type==_VECT){
      (*turtleptr).theta += 90;
      (*turtleptr).radius = 0;
      return update_turtle_state(true,contextptr);
    }
    return _tourne_droite(-g,contextptr);
  }
  static const char _tourne_gauche_s []="tourne_gauche";
  static define_unary_function_eval2 (__tourne_gauche,&_tourne_gauche,_tourne_gauche_s,&printastifunction);
  define_unary_function_ptr5( at_tourne_gauche ,alias_at_tourne_gauche,&__tourne_gauche,0,T_LOGO);

  gen _leve_crayon(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    // logo instruction
    (*turtleptr).mark = false;
    (*turtleptr).radius = 0;
    return update_turtle_state(true,contextptr);
  }
  static const char _leve_crayon_s []="leve_crayon";
  static define_unary_function_eval2 (__leve_crayon,&_leve_crayon,_leve_crayon_s,&printastifunction);
  define_unary_function_ptr5( at_leve_crayon ,alias_at_leve_crayon,&__leve_crayon,0,T_LOGO);

  static const char _penup_s []="penup";
  static define_unary_function_eval (__penup,&_leve_crayon,_penup_s);
  define_unary_function_ptr5( at_penup ,alias_at_penup,&__penup,0,T_LOGO);

  gen _baisse_crayon(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    // logo instruction
    (*turtleptr).mark = true;
    (*turtleptr).radius = 0;
    return update_turtle_state(true,contextptr);
  }
  static const char _baisse_crayon_s []="baisse_crayon";
  static define_unary_function_eval2 (__baisse_crayon,&_baisse_crayon,_baisse_crayon_s,&printastifunction);
  define_unary_function_ptr5( at_baisse_crayon ,alias_at_baisse_crayon,&__baisse_crayon,0,T_LOGO);

  static const char _pendown_s []="pendown";
  static define_unary_function_eval (__pendown,&_baisse_crayon,_pendown_s);
  define_unary_function_ptr5( at_pendown ,alias_at_pendown,&__pendown,0,T_LOGO);

  vector<string> * ecrisptr=0;
  vector<string> & ecristab(){
    if (!ecrisptr)
      ecrisptr=new vector<string>;
    return * ecrisptr;
  }
  gen _ecris(const gen & g,GIAC_CONTEXT){    
    if ( g.type==_STRNG && g.subtype==-1) return  g;
#if 0 //def TURTLETAB
    return gensizeerr("String support does not work with static turtle table");
#endif
    // logo instruction
    (*turtleptr).radius=14;
    if (g.type==_VECT){ 
      vecteur & v =*g._VECTptr;
      int s=int(v.size());
      if (s==2 && v[1].type==_INT_){
	(*turtleptr).radius=absint(v[1].val);
	(*turtleptr).s=ecristab().size();
	ecristab().push_back(gen2string(v.front()));
	return update_turtle_state(false,contextptr);
      }
      if (s==4 && v[1].type==_INT_ && v[2].type==_INT_ && v[3].type==_INT_){
	logo_turtle t=(*turtleptr);
	_leve_crayon(0,contextptr);
	_position(makevecteur(v[2],v[3]),contextptr);
	(*turtleptr).radius=absint(v[1].val);
	(*turtleptr).s=ecristab().size();
	ecristab().push_back(gen2string(v.front()));
	update_turtle_state(false,contextptr);
	(*turtleptr)=t;
	return update_turtle_state(true,contextptr);
      }
    }
    (*turtleptr).s=ecristab().size();
    ecristab().push_back(gen2string(g));
    return update_turtle_state(false,contextptr);
  }
  static const char _ecris_s []="ecris";
  static define_unary_function_eval2 (__ecris,&_ecris,_ecris_s,&printastifunction);
  define_unary_function_ptr5( at_ecris ,alias_at_ecris,&__ecris,0,T_LOGO);

  gen _signe(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    // logo instruction
    return _ecris(makevecteur(g,20,10,10),contextptr);
  }
  static const char _signe_s []="signe";
  static define_unary_function_eval2 (__signe,&_signe,_signe_s,&printastifunction);
  define_unary_function_ptr5( at_signe ,alias_at_signe,&__signe,0,T_LOGO);

  gen _saute(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    _leve_crayon(0,contextptr);
    _avance(g,contextptr);
    return _baisse_crayon(0,contextptr);
  }
  static const char _saute_s []="saute";
  static define_unary_function_eval2 (__saute,&_saute,_saute_s,&printastifunction);
  define_unary_function_ptr5( at_saute ,alias_at_saute,&__saute,0,T_LOGO);

  static const char _jump_s []="jump";
  static define_unary_function_eval2 (__jump,&_saute,_jump_s,&printastifunction);
  define_unary_function_ptr5( at_jump ,alias_at_jump,&__jump,0,T_LOGO);

  gen _pas_de_cote(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    _leve_crayon(0,contextptr);
    _tourne_droite(-90,contextptr);
    _avance(g,contextptr);
    _tourne_droite(90,contextptr);
    return _baisse_crayon(0,contextptr);
  }
  static const char _pas_de_cote_s []="pas_de_cote";
  static define_unary_function_eval2 (__pas_de_cote,&_pas_de_cote,_pas_de_cote_s,&printastifunction);
  define_unary_function_ptr5( at_pas_de_cote ,alias_at_pas_de_cote,&__pas_de_cote,0,T_LOGO);

  static const char _skip_s []="skip";
  static define_unary_function_eval2 (__skip,&_pas_de_cote,_skip_s,&printastifunction);
  define_unary_function_ptr5( at_skip ,alias_at_skip,&__skip,0,T_LOGO);

  gen _cache_tortue(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    // logo instruction
    (*turtleptr).visible=false;
    (*turtleptr).radius = 0;
    return update_turtle_state(true,contextptr);
  }
  static const char _cache_tortue_s []="cache_tortue";
  static define_unary_function_eval2 (__cache_tortue,&_cache_tortue,_cache_tortue_s,&printastifunction);
  define_unary_function_ptr5( at_cache_tortue ,alias_at_cache_tortue,&__cache_tortue,0,T_LOGO);

  static const char _hideturtle_s []="hideturtle";
  static define_unary_function_eval (__hideturtle,&_cache_tortue,_hideturtle_s);
  define_unary_function_ptr5( at_hideturtle ,alias_at_hideturtle,&__hideturtle,0,true);

  gen _montre_tortue(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    // logo instruction
    (*turtleptr).visible=true;
    (*turtleptr).radius = 0;
    return update_turtle_state(true,contextptr);
  }
  static const char _montre_tortue_s []="montre_tortue";
  static define_unary_function_eval2 (__montre_tortue,&_montre_tortue,_montre_tortue_s,&printastifunction);
  define_unary_function_ptr5( at_montre_tortue ,alias_at_montre_tortue,&__montre_tortue,0,T_LOGO);

  static const char _showturtle_s []="showturtle";
  static define_unary_function_eval (__showturtle,&_montre_tortue,_showturtle_s);
  define_unary_function_ptr5( at_showturtle ,alias_at_showturtle,&__showturtle,0,true);


  gen _repete(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    if (g.type!=_VECT || g._VECTptr->size()<2)
      return gensizeerr(contextptr);
    // logo instruction
    vecteur v = *g._VECTptr;
    v[0]=eval(v[0],contextptr);
    if (v.front().type!=_INT_)
      return gentypeerr(contextptr);
    gen prog=vecteur(v.begin()+1,v.end());
    int i=absint(v.front().val);
    gen res;
    for (int j=0;j<i;++j){
      res=eval(prog,contextptr);
    }
    return res;
  }
  static const char _repete_s []="repete";
  static define_unary_function_eval_quoted (__repete,&_repete,_repete_s);
  define_unary_function_ptr5( at_repete ,alias_at_repete,&__repete,_QUOTE_ARGUMENTS,T_RETURN);

  gen _crayon(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    if (g.type==_STRNG) return _crayon(gen(*g._STRNGptr,contextptr),contextptr);
    // logo instruction
    if (g.type==_VECT && g._VECTptr->size()==3)
      return _crayon(_rgb(g,contextptr),contextptr);
    if (g.type!=_INT_){
      gen res=(*turtleptr).color;
      res.subtype=_INT_COLOR;
      return res;
    }
    (*turtleptr).color=g.val;
    (*turtleptr).radius = 0;
    return update_turtle_state(true,contextptr);
  }
  static const char _crayon_s []="crayon";
  static define_unary_function_eval2 (__crayon,&_crayon,_crayon_s,&printastifunction);
  define_unary_function_ptr5( at_crayon ,alias_at_crayon,&__crayon,0,T_LOGO);

  static const char _pencolor_s []="pencolor";
  static define_unary_function_eval (__pencolor,&_crayon,_pencolor_s);
  define_unary_function_ptr5( at_pencolor ,alias_at_pencolor,&__pencolor,0,T_LOGO);

  gen _efface_logo(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    if (g.type==_INT_){
      _crayon(int(FL_WHITE),contextptr);
      _recule(g,contextptr);
      return _crayon(0,contextptr);
    }
    // logo instruction
    (*turtleptr) = logo_turtle();
#ifdef TURTLETAB
    turtle_stack_size=0;
#else
    turtle_stack().clear();
#endif
    ecristab().clear();
    return update_turtle_state(true,contextptr);
  }
  static const char _efface_logo_s []="efface";
  static define_unary_function_eval2 (__efface_logo,&_efface_logo,_efface_logo_s,&printastifunction);
  define_unary_function_ptr5( at_efface_logo ,alias_at_efface_logo,&__efface_logo,0,T_LOGO);

  static const char _efface_s []="efface";
  static define_unary_function_eval2 (__efface,&_efface_logo,_efface_s,&printastifunction);
  define_unary_function_ptr5( at_efface ,alias_at_efface,&__efface,0,T_LOGO);

  static const char _reset_s []="reset";
  static define_unary_function_eval2 (__reset,&_efface_logo,_reset_s,&printastifunction);
  define_unary_function_ptr5( at_reset ,alias_at_reset,&__reset,0,T_LOGO);

  static const char _clearscreen_s []="clearscreen";
  static define_unary_function_eval2 (__clearscreen,&_efface_logo,_clearscreen_s,&printastifunction);
  define_unary_function_ptr5( at_clearscreen ,alias_at_clearscreen,&__clearscreen,0,T_LOGO);

  gen _debut_enregistrement(const gen &g,GIAC_CONTEXT){
    return undef;
  }
  static const char _debut_enregistrement_s []="debut_enregistrement";
  static define_unary_function_eval2 (__debut_enregistrement,&_debut_enregistrement,_debut_enregistrement_s,&printastifunction);
  define_unary_function_ptr5( at_debut_enregistrement ,alias_at_debut_enregistrement,&__debut_enregistrement,0,T_LOGO);

  static const char _fin_enregistrement_s []="fin_enregistrement";
  static define_unary_function_eval2 (__fin_enregistrement,&_debut_enregistrement,_fin_enregistrement_s,&printastifunction);
  define_unary_function_ptr5( at_fin_enregistrement ,alias_at_fin_enregistrement,&__fin_enregistrement,0,T_LOGO);

  static const char _turtle_stack_s []="turtle_stack";
  static define_unary_function_eval2 (__turtle_stack,&_debut_enregistrement,_turtle_stack_s,&printastifunction);
  define_unary_function_ptr5( at_turtle_stack ,alias_at_turtle_stack,&__turtle_stack,0,T_LOGO);

  gen _vers(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    // logo instruction
    if (g.type!=_VECT || g._VECTptr->size()!=2)
      return gensizeerr(contextptr);
    gen x=evalf_double(g._VECTptr->front(),1,contextptr),
      y=evalf_double(g._VECTptr->back(),1,contextptr);
    if (x.type!=_DOUBLE_ || y.type!=_DOUBLE_)
      return gensizeerr(contextptr);
    double xv=x._DOUBLE_val,yv=y._DOUBLE_val,xt=(*turtleptr).x,yt=(*turtleptr).y;
    double theta=atan2(yv-yt,xv-xt);
    return _cap(theta*180/M_PI,contextptr);
  }
  static const char _vers_s []="vers";
  static define_unary_function_eval2 (__vers,&_vers,_vers_s,&printastifunction);
  define_unary_function_ptr5( at_vers ,alias_at_vers,&__vers,0,T_LOGO);

  static int find_radius(const gen & g,int & r,int & theta2,bool & direct){
    int radius;
    direct=true;
    theta2 = 360 ;
    // logo instruction
    if (g.type==_VECT && !g._VECTptr->empty()){
      vecteur v = *g._VECTptr;
      bool seg=false;
      if (v.back()==at_segment){
	v.pop_back();
	seg=true;
      }
      if (v.size()<2)
	return RAND_MAX; // setdimerr(contextptr);
      if (v[0].type==_INT_)
	r=v[0].val;
      else {
	gen v0=evalf_double(v[0],1,context0);
	if (v0.type==_DOUBLE_)
	  r=int(v0._DOUBLE_val+0.5);
	else 
	  return RAND_MAX; // setsizeerr(contextptr);
      }
      if (r<0){
	r=-r;
	direct=false;
      }
      int theta1;
      if (v[1].type==_DOUBLE_)
	theta1=int(v[1]._DOUBLE_val+0.5);
      else { 
	if (v[1].type==_INT_)
	  theta1=v[1].val;
	else return RAND_MAX; // setsizeerr(contextptr);
      }
      while (theta1<0)
	theta1 += 360;
      if (v.size()>=3){
	if (v[2].type==_DOUBLE_)
	  theta2 = int(v[2]._DOUBLE_val+0.5);
	else {
	  if (v[2].type==_INT_)
	    theta2 = v[2].val;
	  else return RAND_MAX; // setsizeerr(contextptr);
	}
	while (theta2<0)
	  theta2 += 360;
	radius = giacmin(r,512) | (giacmin(theta1,360) << 9) | (giacmin(theta2,360) << 18 ) | (seg?(1<<28):0);
      }
      else {// angle 1=0
	theta2 = theta1;
	if (theta2<0)
	  theta2 += 360;
	radius = giacmin(r,512) | (giacmin(theta2,360) << 18 ) | (seg?(1<<28):0);
      }
      return radius;
    }
    radius = 10;
    if (g.type==_INT_)
      radius= (r=g.val);
    if (g.type==_DOUBLE_)
      radius= (r=int(g._DOUBLE_val));
    if (radius<=0){
      radius = -radius;
      direct=false;
    }
    radius = giacmin(radius,512 )+(360 << 18) ; // 2nd angle = 360 degrees
    return radius;
  }

  static void turtle_move(int r,int theta2,GIAC_CONTEXT){
    double theta0;
    if ((*turtleptr).direct)
      theta0=(*turtleptr).theta-90;
    else {
      theta0=(*turtleptr).theta+90;
      theta2=-theta2;
    }
    (*turtleptr).x += r*(std::cos(M_PI/180*(theta2+theta0))-std::cos(M_PI/180*theta0));
    (*turtleptr).y += r*(std::sin(M_PI/180*(theta2+theta0))-std::sin(M_PI/180*theta0));
    (*turtleptr).theta = (*turtleptr).theta+theta2 ;
    if ((*turtleptr).theta<0)
      (*turtleptr).theta += 360;
    if ((*turtleptr).theta>360)
      (*turtleptr).theta -= 360;
  }

  gen _rond(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    int r,theta2,tmpr;
    tmpr=find_radius(g,r,theta2,(*turtleptr).direct);
    if (tmpr==RAND_MAX)
      return gensizeerr(contextptr);
    (*turtleptr).radius=tmpr;
    turtle_move(r,theta2,contextptr);
    return update_turtle_state(true,contextptr);
  }
  static const char _rond_s []="rond";
  static define_unary_function_eval2 (__rond,&_rond,_rond_s,&printastifunction);
  define_unary_function_ptr5( at_rond ,alias_at_rond,&__rond,0,T_LOGO);

  gen _disque(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    int r,theta2,tmpr=find_radius(g,r,theta2,(*turtleptr).direct);
    if (tmpr==RAND_MAX)
      return gensizeerr(contextptr);
    (*turtleptr).radius=tmpr;
    turtle_move(r,theta2,contextptr);
    (*turtleptr).radius += 1 << 27;
    return update_turtle_state(true,contextptr);
  }
  static const char _disque_s []="disque";
  static define_unary_function_eval2 (__disque,&_disque,_disque_s,&printastifunction);
  define_unary_function_ptr5( at_disque ,alias_at_disque,&__disque,0,T_LOGO);

  gen _disque_centre(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    int r,theta2;
    bool direct;
    int radius=find_radius(g,r,theta2,direct);
    if (radius==RAND_MAX)
      return gensizeerr(contextptr);
    r=absint(r);
    _saute(r,contextptr);
    _tourne_gauche(direct?90:-90,contextptr);
    (*turtleptr).radius = radius;
    (*turtleptr).direct=direct;
    turtle_move(r,theta2,contextptr);
    (*turtleptr).radius += 1 << 27;
    update_turtle_state(true,contextptr);
    _tourne_droite(direct?90:-90,contextptr);
    return _saute(-r,contextptr);
  }
  static const char _disque_centre_s []="disque_centre";
  static define_unary_function_eval2 (__disque_centre,&_disque_centre,_disque_centre_s,&printastifunction);
  define_unary_function_ptr5( at_disque_centre ,alias_at_disque_centre,&__disque_centre,0,T_LOGO);

  gen _polygone_rempli(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    if (g.type==_INT_){
      (*turtleptr).radius=-absint(g.val);
      if ((*turtleptr).radius<-1)
	return update_turtle_state(true,contextptr);
    }
    return gensizeerr(gettext("Integer argument >= 2"));
  }
  static const char _polygone_rempli_s []="polygone_rempli";
  static define_unary_function_eval2 (__polygone_rempli,&_polygone_rempli,_polygone_rempli_s,&printastifunction);
  define_unary_function_ptr5( at_polygone_rempli ,alias_at_polygone_rempli,&__polygone_rempli,0,T_LOGO);

  gen _rectangle_plein(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    gen gx=g,gy=g;
    if (g.type==_VECT && g._VECTptr->size()==2){
      gx=g._VECTptr->front();
      gy=g._VECTptr->back();
    }
    for (int i=0;i<2;++i){
      _avance(gx,contextptr);
      _tourne_droite(-90,contextptr);
      _avance(gy,contextptr);
      _tourne_droite(-90,contextptr);
    }
    //for (int i=0;i<turtle_stack().size();++i){ *logptr(contextptr) << turtle2gen(turtle_stack()[i]) <<endl;}
    return _polygone_rempli(-8,contextptr);
  }
  static const char _rectangle_plein_s []="rectangle_plein";
  static define_unary_function_eval2 (__rectangle_plein,&_rectangle_plein,_rectangle_plein_s,&printastifunction);
  define_unary_function_ptr5( at_rectangle_plein ,alias_at_rectangle_plein,&__rectangle_plein,0,T_LOGO);

  gen _triangle_plein(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    gen gx=g,gy=g,gtheta=60;
    if (g.type==_VECT && g._VECTptr->size()>=2){
      vecteur & v=*g._VECTptr;
      gx=v.front();
      gy=v[1];
      gtheta=90;
      if (v.size()>2)
	gtheta=v[2];
    }
    logo_turtle t=(*turtleptr);
    _avance(gx,contextptr);
    double save_x=(*turtleptr).x,save_y=(*turtleptr).y;
    _recule(gx,contextptr);
    _tourne_gauche(gtheta,contextptr);
    _avance(gy,contextptr);
    (*turtleptr).x=save_x;
    (*turtleptr).y=save_y;
    update_turtle_state(true,contextptr);
    (*turtleptr)=t;
    (*turtleptr).radius=0;
    update_turtle_state(true,contextptr);
    return _polygone_rempli(-3,contextptr);
  }
  static const char _triangle_plein_s []="triangle_plein";
  static define_unary_function_eval2 (__triangle_plein,&_triangle_plein,_triangle_plein_s,&printastifunction);
  define_unary_function_ptr5( at_triangle_plein ,alias_at_triangle_plein,&__triangle_plein,0,T_LOGO);

  gen _dessine_tortue(const gen & g,GIAC_CONTEXT){
    if ( g.type==_STRNG && g.subtype==-1) return  g;
    // logo instruction
    /*
      _triangle_plein(makevecteur(17,5));
      _tourne_droite(90);
      _triangle_plein(makevecteur(5,17));
      return _tourne_droite(-90);
    */
    double save_x=(*turtleptr).x,save_y=(*turtleptr).y;
    _tourne_droite(90,contextptr);
    _avance(5,contextptr);
    _tourne_gauche(106,contextptr);
    _avance(18,contextptr);
    _tourne_gauche(148,contextptr);
    _avance(18,contextptr);
    _tourne_gauche(106,contextptr);
    _avance(5,contextptr);
    (*turtleptr).x=save_x; (*turtleptr).y=save_y;
    gen res(_tourne_gauche(90,contextptr));
    if (is_one(g))
      return res;
    return _polygone_rempli(-9,contextptr);
  }
  static const char _dessine_tortue_s []="dessine_tortue";
  static define_unary_function_eval2 (__dessine_tortue,&_dessine_tortue,_dessine_tortue_s,&printastifunction);
  define_unary_function_ptr5( at_dessine_tortue ,alias_at_dessine_tortue,&__dessine_tortue,0,T_LOGO);
  
#ifndef NO_NAMESPACE_GIAC
} // namespace giac
#endif // ndef NO_NAMESPACE_GIAC


#ifndef NO_NAMESPACE_XCAS
namespace xcas {
#endif // ndef NO_NAMESPACE_XCAS
  void drawRectangle(int x,int y,int w,int h,int c){
    draw_rectangle(x,y,w,h,c,context0);
  }
  void draw_rectangle(int x,int y,int w,int h,int c){
    draw_rectangle(x,y,w,h,c,context0);
  }
  void draw_line(int x0,int y0,int x1,int y1,int c){
    draw_line(x0,y0,x1,y1,c,context0);
  }
  void draw_circle(int xc,int yc,int r,int color,bool q1=true,bool q2=true,bool q3=true,bool q4=true){
    draw_circle(xc,yc,r,color,q1,q2,q3,q4,context0);
  }
  void draw_filled_circle(int xc,int yc,int r,int color,bool left=true,bool right=true){
    draw_filled_circle(xc,yc,r,color,left,right,context0);
  }
  void draw_filled_polygon(std::vector< vector<int> > &L,int xmin,int xmax,int ymin,int ymax,int color){
    draw_filled_polygon(L,xmin,xmax,ymin,ymax,color,context0);
  }
  unsigned max_prettyprint_equation=256;

  // make a free copy of g
  gen Equation_copy(const gen & g){
    if (g.type==_EQW)
      return *g._EQWptr;
    if (g.type!=_VECT)
      return g;
    vecteur & v = *g._VECTptr;
    const_iterateur it=v.begin(),itend=v.end();
    vecteur res;
    res.reserve(itend-it);
    for (;it!=itend;++it)
      res.push_back(Equation_copy(*it));
    return gen(res,g.subtype);
  }

  // matrix/list select
  bool do_select(gen & eql,bool select,gen & value){
    if (eql.type==_VECT && !eql._VECTptr->empty()){
      vecteur & v=*eql._VECTptr;
      size_t s=v.size();
      if (v[s-1].type!=_EQW)
	return false;
      v[s-1]._EQWptr->selected=select;
      gen sommet=v[s-1]._EQWptr->g;
      --s;
      vecteur args(s);
      for (size_t i=0;i<s;++i){
	if (!do_select(v[i],select,args[i]))
	  return false;
	if (args[i].type==_EQW)
	  args[i]=args[i]._EQWptr->g;
      }
      gen va=s==1?args[0]:gen(args,_SEQ__VECT);
      if (sommet.type==_FUNC)
	va=symbolic(*sommet._FUNCptr,va);
      else
	va=sommet(va,context0);
      //cout << "va " << va << endl;
      value=*v[s]._EQWptr;
      value._EQWptr->g=va;
      //cout << "value " << value << endl;
      return true;
    }
    if (eql.type!=_EQW)
      return false;
    eql._EQWptr->selected=select;
    value=eql;
    return true;
  }
  
  bool Equation_box_sizes(const gen & g,int & l,int & h,int & x,int & y,attributs & attr,bool & selected){
    if (g.type==_EQW){
      eqwdata & w=*g._EQWptr;
      x=w.x;
      y=w.y;
      l=w.dx;
      h=w.dy;
      selected=w.selected;
      attr=w.eqw_attributs;
      //cout << g << endl;
      return true;
    }
    else {
      if (g.type!=_VECT || g._VECTptr->empty() ){
	l=0;
	h=0;
	x=0;
	y=0;
	attr=attributs(0,0,0);
	selected=false;
	return true;
      }
      gen & g1=g._VECTptr->back();
      Equation_box_sizes(g1,l,h,x,y,attr,selected);
      return false;
    }
  }

  // return true if g has some selection inside, gsel points to the selection
  bool Equation_adjust_xy(gen & g,int & xleft,int & ytop,int & xright,int & ybottom,gen * & gsel,gen * & gselparent,int &gselpos,std::vector<int> * goto_ptr){
    gsel=0;
    gselparent=0;
    gselpos=0;
    int x,y,w,h;
    attributs f(0,0,0);
    bool selected;
    Equation_box_sizes(g,w,h,x,y,f,selected);
    if ( (g.type==_EQW__VECT) || selected ){ // terminal or selected
      xleft=x;
      ybottom=y;
      if (selected){ // g is selected
	ytop=y+h;
	xright=x+w;
	gsel =  &g;
	//cout << "adjust " << *gsel << endl;
	return true;
      }
      else { // no selection
	xright=x;
	ytop=y;
	return false;
      }
    }
    if (g.type!=_VECT)
      return false;
    // last not selected, recurse
    iterateur it=g._VECTptr->begin(),itend=g._VECTptr->end()-1;
    for (;it!=itend;++it){
      if (Equation_adjust_xy(*it,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos,goto_ptr)){
	if (goto_ptr){
	  goto_ptr->push_back(it-g._VECTptr->begin());
	  //cout << g << ":" << *goto_ptr << endl;
	}
	if (gsel==&*it){
	  // check next siblings
	  
	  gselparent= &g;
	  gselpos=it-g._VECTptr->begin();
	  //cout << "gselparent " << g << endl;
	}
	return true;
      }
    }
    return false;
  }
 
  // select or deselect part of the current eqution
  // This is done *in place*
  void Equation_select(gen & g,bool select){
    if (g.type==_EQW){
      eqwdata & e=*g._EQWptr;
      e.selected=select;
    }
    if (g.type!=_VECT)
      return;
    vecteur & v=*g._VECTptr;
    iterateur it=v.begin(),itend=v.end();
    for (;it!=itend;++it)
      Equation_select(*it,select);
  }

  // decrease selection (like HP49 eqw Down key)
  int eqw_select_down(gen & g){
    int xleft,ytop,xright,ybottom,gselpos;
    int newxleft,newytop,newxright,newybottom;
    gen * gsel,*gselparent;
    if (Equation_adjust_xy(g,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos)){
      //cout << "select down before " << *gsel << endl;
      if (gsel->type==_VECT && !gsel->_VECTptr->empty()){
	Equation_select(*gsel,false);
	Equation_select(gsel->_VECTptr->front(),true);
	//cout << "select down after " << *gsel << endl;
	Equation_adjust_xy(g,newxleft,newytop,newxright,newybottom,gsel,gselparent,gselpos);
	return newytop-ytop;
      }
    }
    return 0;
  }

  int eqw_select_up(gen & g){
    int xleft,ytop,xright,ybottom,gselpos;
    int newxleft,newytop,newxright,newybottom;
    gen * gsel,*gselparent;
    if (Equation_adjust_xy(g,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos) && gselparent){
      Equation_select(*gselparent,true);
      //cout << "gselparent " << *gselparent << endl;
      Equation_adjust_xy(g,newxleft,newytop,newxright,newybottom,gsel,gselparent,gselpos);
      return newytop-ytop;
    }
    return false;
  }

  // exchange==0 move selection to left or right sibling, ==2 add left or right
  // sibling, ==1 exchange selection with left or right sibling
  int eqw_select_leftright(Equation & eq,bool left,int exchange,GIAC_CONTEXT){
    gen & g=eq.data;
    int xleft,ytop,xright,ybottom,gselpos;
    int newxleft,newytop,newxright,newybottom;
    gen * gsel,*gselparent;
    vector<int> goto_sel;
    if (Equation_adjust_xy(g,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos,&goto_sel) && gselparent && gselparent->type==_VECT){
      vecteur & gselv=*gselparent->_VECTptr;
      int n=gselv.size()-1,gselpos_orig=gselpos;
      if (n<1) return 0;
      if (left) {
	if (gselpos==0)
	  gselpos=n-1;
	else
	  gselpos--;
      }
      else {
	if (gselpos==n-1)
	  gselpos=0;
	else
	  gselpos++;
      }
      if (exchange==1){ // exchange gselpos_orig and gselpos
	swapgen(gselv[gselpos],gselv[gselpos_orig]);
	gsel=&gselv[gselpos_orig];
	gen value;
	if (xcas::do_select(*gsel,true,value) && value.type==_EQW)
	  replace_selection(eq,value._EQWptr->g,gsel,&goto_sel,contextptr);
      }
      else {
	// increase selection to next sibling possible for + and * only
	if (n>2 && exchange==2 && gselv[n].type==_EQW && (gselv[n]._EQWptr->g==at_plus || gselv[n]._EQWptr->g==at_prod)){
	  gen value1, value2,tmp;
	  if (gselpos_orig<gselpos)
	    swapint(gselpos_orig,gselpos);
	  // now gselpos<gselpos_orig
	  xcas::do_select(gselv[gselpos_orig],true,value1);
	  xcas::do_select(gselv[gselpos],true,value2);
	  if (value1.type==_EQW && value2.type==_EQW){
	    tmp=gselv[n]._EQWptr->g==at_plus?value1._EQWptr->g+value2._EQWptr->g:value1._EQWptr->g*value2._EQWptr->g;
	    gselv.erase(gselv.begin()+gselpos_orig);
	    replace_selection(eq,tmp,&gselv[gselpos],&goto_sel,contextptr);
	  }
	}
	else {
	  Equation_select(*gselparent,false);
	  gen & tmp=(*gselparent->_VECTptr)[gselpos];
	  Equation_select(tmp,true);
	}
      }
      Equation_adjust_xy(g,newxleft,newytop,newxright,newybottom,gsel,gselparent,gselpos);
      return newxleft-xleft;
    }
    return 0;
  }

  bool eqw_select(const gen & eq,int l,int c,bool select,gen & value){
    value=undef;
    if (l<0 || eq.type!=_VECT || eq._VECTptr->size()<=l)
      return false;
    gen & eql=(*eq._VECTptr)[l];
    if (c<0)
      return do_select(eql,select,value);
    if (eql.type!=_VECT || eql._VECTptr->size()<=c)
      return false;
    gen & eqlc=(*eql._VECTptr)[c];
    return do_select(eqlc,select,value);
  }

  gen Equation_compute_size(const gen & g,const attributs & a,int windowhsize,GIAC_CONTEXT);
  
  // void Bdisp_MMPrint(int x, int y, const char* string, int mode_flags, int xlimit, int P6, int P7, int color, int back_color, int writeflag, int P11); 
  // void PrintCXY(int x, int y, const char *cptr, int mode_flags, int P5, int color, int back_color, int P8, int P9)
  // void PrintMini( int* x, int* y, const char* string, int mode_flags, unsigned int xlimit, int P6, int P7, int color, int back_color, int writeflag, int P11) 
  void text_print(int fontsize,const char * s,int x,int y,int c=COLOR_BLACK,int bg=COLOR_WHITE,int mode=0){
    // *logptr(contextptr) << x << " " << y << " " << fontsize << " " << s << endl; return;
    c=(unsigned short) c;
    if (x>LCD_WIDTH_PX) return;
    int ss=strlen(s);
    if (ss==1 && s[0]==0x1e){ // arrow for limit
      if (mode==4)
	c=bg;
      draw_line(x,y-4,x+fontsize/2,y-4,c);
      draw_line(x,y-3,x+fontsize/2,y-3,c);
      draw_line(x+fontsize/2-4,y,x+fontsize/2,y-4,c);
      draw_line(x+fontsize/2-3,y,x+fontsize/2+1,y-4,c);
      draw_line(x+fontsize/2-4,y-7,x+fontsize/2,y-3,c);   
      draw_line(x+fontsize/2-3,y-7,x+fontsize/2+1,y-3,c);   
      return;
    }
    if (ss==2 && strcmp(s,"pi")==0){
      if (mode==4){
	drawRectangle(x,y+2-fontsize,fontsize,fontsize,c);
	c=bg;
      }
      draw_line(x+fontsize/3-1,y+1,x+fontsize/3,y+6-fontsize,c);
      draw_line(x+fontsize/3-2,y+1,x+fontsize/3-1,y+6-fontsize,c);
      draw_line(x+2*fontsize/3,y+1,x+2*fontsize/3,y+6-fontsize,c);
      draw_line(x+2*fontsize/3+1,y+1,x+2*fontsize/3+1,y+6-fontsize,c);
      draw_line(x+2,y+6-fontsize,x+fontsize,y+6-fontsize,c);
      draw_line(x+2,y+5-fontsize,x+fontsize,y+5-fontsize,c);
      return;
    }
    if (fontsize>=16 && ss==2 && s[0]==char(0xe5) && (s[1]==char(0xea) || s[1]==char(0xeb))) // special handling for increasing and decreasing in tabvar output
      fontsize=18;
    if (fontsize>=18){
      y -= 16;// status area shift
      numworks_giac_draw_string(x,y,mode==4?bg:c,mode==4?c:bg,s);
      // PrintMini(&x,&y,(unsigned char *)s,mode,0xffffffff,0,0,c,bg,1,0);
      return;
    }
    y -= 12;
    numworks_giac_draw_string_small(x,y,mode==4?bg:c,mode==4?c:bg,s);// PrintMiniMini( &x, &y, (unsigned char *)s, mode,c, 0 );
    return;
  }
  
  int text_width(int fontsize,const char * s){
    if (fontsize>=18)
      return strlen(s)*11;
    return strlen(s)*7;
  }

  void fl_arc(int x,int y,int rx,int ry,int theta1_deg,int theta2_deg,int c=COLOR_BLACK){
    rx/=2;
    ry/=2;
    // *logptr(contextptr) << "theta " << theta1_deg << " " << theta2_deg << endl;
    if (ry==rx){
      if (theta2_deg-theta1_deg==360){
	draw_circle(x+rx,y+rx,rx,c);
	return;
      }
      if (theta1_deg==0 && theta2_deg==180){
	draw_circle(x+rx,y+rx,rx,c,true,true,false,false);
	return;
      }
      if (theta1_deg==180 && theta2_deg==360){
	draw_circle(x+rx,y+rx,rx,c,false,false,true,true);
	return;
      }
    }
    // *logptr(contextptr) << "draw_arc" << theta1_deg*M_PI/180. << " " << theta2_deg*M_PI/180. << endl;
    draw_arc(x+rx,y+ry,rx,ry,c,theta1_deg*M_PI/180.,theta2_deg*M_PI/180.,context0);
  }

  void fl_pie(int x,int y,int rx,int ry,int theta1_deg,int theta2_deg,int c=COLOR_BLACK,bool segment=false){
    //cout << "fl_pie " << theta1_deg << " " << theta2_deg << " " << c << endl;
    if (!segment && ry==rx){
      if (theta2_deg-theta1_deg>=360){
	rx/=2;
	draw_filled_circle(x+rx,y+rx,rx,c);
	return;
      }
      if (theta1_deg==-90 && theta2_deg==90){
	rx/=2;
	draw_filled_circle(x+rx,y+rx,rx,c,false,true);
	return;
      }
      if (theta1_deg==90 && theta2_deg==270){
	rx/=2;
	draw_filled_circle(x+rx,y+rx,rx,c,true,false);
	return;
      }
    }
    // approximation by a filled polygon
    // points: (x,y), (x+rx*cos(theta)/2,y+ry*sin(theta)/2) theta=theta1..theta2
    while (theta2_deg<theta1_deg)
      theta2_deg+=360;
    if (theta2_deg-theta1_deg>=360){
      theta1_deg=0;
      theta2_deg=360;
    }
    int N0=theta2_deg-theta1_deg+1;
    // reduce N if rx or ry is small
    double red=double(rx)/LCD_WIDTH_PX*double(ry)/LCD_HEIGHT_PX;
    if (red>1) red=1;
    if (red<0.1) red=0.1;
    int N=red*N0;
    if (N<5)
      N=N0>5?5:N0;
    if (N<2)
      N=2;
    vector< vector<int> > v(segment?N+1:N+2,vector<int>(2));
    x += rx/2;
    y += ry/2;
    int i=0;
    if (!segment){
      v[0][0]=x;
      v[0][1]=y;
      ++i;
    }
    double theta=theta1_deg*M_PI/180;
    double thetastep=(theta2_deg-theta1_deg)*M_PI/(180*(N-1));
    for (;i<v.size()-1;++i){
      v[i][0]=int(x+rx*std::cos(theta)/2+.5);
      v[i][1]=int(y-ry*std::sin(theta)/2+.5); // y is inverted
      theta += thetastep;
    }
    v.back()=v.front();
    draw_filled_polygon(v,0,LCD_WIDTH_PX,24,LCD_HEIGHT_PX,c);
  }

  bool binary_op(const unary_function_ptr & u){
    const unary_function_ptr binary_op_tab_ptr []={*at_plus,*at_prod,*at_pow,*at_and,*at_ou,*at_xor,*at_different,*at_same,*at_equal,*at_unit,*at_compose,*at_composepow,*at_deuxpoints,*at_tilocal,*at_pointprod,*at_pointdivision,*at_pointpow,*at_division,*at_normalmod,*at_minus,*at_intersect,*at_union,*at_interval,*at_inferieur_egal,*at_inferieur_strict,*at_superieur_egal,*at_superieur_strict,*at_equal2,0};
    return equalposcomp(binary_op_tab_ptr,u);
  }
  
  eqwdata Equation_total_size(const gen & g){
    if (g.type==_EQW)
      return *g._EQWptr;
    if (g.type!=_VECT || g._VECTptr->empty())
      return eqwdata(0,0,0,0,attributs(0,0,0),undef);
    return Equation_total_size(g._VECTptr->back());
  }

  // find smallest value of y and height
  void Equation_y_dy(const gen & g,int & y,int & dy){
    y=0; dy=0;
    if (g.type==_EQW){
      y=g._EQWptr->y;
      dy=g._EQWptr->dy;
    }
    if (g.type==_VECT){
      iterateur it=g._VECTptr->begin(),itend=g._VECTptr->end();
      for (;it!=itend;++it){
	int Y,dY;
	Equation_y_dy(*it,Y,dY);
	// Y, Y+dY and y,y+dy
	int ymax=giacmax(y+dy,Y+dY);
	if (Y<y)
	  y=Y;
	dy=ymax-y;
      }
    }
  }

  void Equation_translate(gen & g,int deltax,int deltay){
    if (g.type==_EQW){
      g._EQWptr->x += deltax;
      g._EQWptr->y += deltay;
      g._EQWptr->baseline += deltay;
      return ;
    }
    if (g.type!=_VECT)
      setsizeerr();
    vecteur & v=*g._VECTptr;
    iterateur it=v.begin(),itend=v.end();
    for (;it!=itend;++it)
      Equation_translate(*it,deltax,deltay);
  }

  gen Equation_change_attributs(const gen & g,const attributs & newa){
    if (g.type==_EQW){
      gen res(*g._EQWptr);
      res._EQWptr->eqw_attributs = newa;
      return res;
    }
    if (g.type!=_VECT)
      return gensizeerr();
    vecteur v=*g._VECTptr;
    iterateur it=v.begin(),itend=v.end();
    for (;it!=itend;++it)
      *it=Equation_change_attributs(*it,newa);
    return gen(v,g.subtype);
  }

  vecteur Equation_subsizes(const gen & arg,const attributs & a,int windowhsize,GIAC_CONTEXT){
    vecteur v;
    if ( (arg.type==_VECT) && ( (arg.subtype==_SEQ__VECT) 
				// || (!ckmatrix(arg)) 
				) ){
      const_iterateur it=arg._VECTptr->begin(),itend=arg._VECTptr->end();
      for (;it!=itend;++it)
	v.push_back(Equation_compute_size(*it,a,windowhsize,contextptr));
    }
    else {
      v.push_back(Equation_compute_size(arg,a,windowhsize,contextptr));
    }
    return v;
  }

  // vertical merge with same baseline
  // for vertical merge of hp,yp at top (like ^) add fontsize to yp
  // at bottom (like lower bound of int) substract fontsize from yp
  void Equation_vertical_adjust(int hp,int yp,int & h,int & y){
    int yf=min(y,yp);
    h=max(y+h,yp+hp)-yf;
    y=yf;
  }

  gen Equation_compute_symb_size(const gen & g,const attributs & a,int windowhsize,GIAC_CONTEXT){
    if (g.type!=_SYMB)
      return Equation_compute_size(g,a,windowhsize,contextptr);
    unary_function_ptr & u=g._SYMBptr->sommet;
    gen arg=g._SYMBptr->feuille,rootof_value;
    if (u==at_makevector){
      vecteur v(1,arg);
      if (arg.type==_VECT)
	v=*arg._VECTptr;
      iterateur it=v.begin(),itend=v.end();
      for (;it!=itend;++it){
	if ( (it->type==_SYMB) && (it->_SYMBptr->sommet==at_makevector) )
	  *it=_makevector(it->_SYMBptr->feuille,contextptr);
      }
      return Equation_compute_size(v,a,windowhsize,contextptr);
    }
    if (u==at_makesuite){
      if (arg.type==_VECT)
	return Equation_compute_size(gen(*arg._VECTptr,_SEQ__VECT),a,windowhsize,contextptr);
      else
	return Equation_compute_size(arg,a,windowhsize,contextptr);
    }
    if (u==at_sqrt)
      return Equation_compute_size(symb_pow(arg,plus_one_half),a,windowhsize,contextptr);
    if (u==at_division){
      if (arg.type!=_VECT || arg._VECTptr->size()!=2)
	return Equation_compute_size(arg,a,windowhsize,contextptr);
      gen tmp=Tfraction<gen>(arg._VECTptr->front(),arg._VECTptr->back());
      return Equation_compute_size(tmp,a,windowhsize,contextptr);
    }
    if (u==at_prod){
      gen n,d;
      if (rewrite_prod_inv(arg,n,d)){
	if (n.is_symb_of_sommet(at_neg))
	  return Equation_compute_size(symbolic(at_neg,Tfraction<gen>(-n,d)),a,windowhsize,contextptr);
	return Equation_compute_size(Tfraction<gen>(n,d),a,windowhsize,contextptr);
      }
    }
    if (u==at_inv){
      if ( (is_integer(arg) && is_positive(-arg,contextptr))
	   || (arg.is_symb_of_sommet(at_neg)))
	return Equation_compute_size(symbolic(at_neg,Tfraction<gen>(plus_one,-arg)),a,windowhsize,contextptr);
      return Equation_compute_size(Tfraction<gen>(plus_one,arg),a,windowhsize,contextptr);
    }
    if (u==at_expr && arg.type==_VECT && arg.subtype==_SEQ__VECT && arg._VECTptr->size()==2 && arg._VECTptr->back().type==_INT_){
      gen varg1=Equation_compute_size(arg._VECTptr->front(),a,windowhsize,contextptr);
      eqwdata vv(Equation_total_size(varg1));
      gen varg2=eqwdata(0,0,0,0,a,arg._VECTptr->back());
      vecteur v12(makevecteur(varg1,varg2));
      v12.push_back(eqwdata(vv.dx,vv.dy,0,vv.y,a,at_expr,0));
      return gen(v12,_SEQ__VECT);
    }
    int llp=int(text_width(a.fontsize,("(")))-1;
    int lrp=llp;
    int lc=int(text_width(a.fontsize,(",")));
    string us=u.ptr()->s;
    int ls=int(text_width(a.fontsize,(us.c_str())));
    // if (isalpha(u.ptr()->s[0])) ls += 1;
    if (u==at_abs)
      ls = 2;
    // special cases first int, sigma, /, ^
    // and if printed as printsommetasoperator
    // otherwise print with usual functional notation
    int x=0;
    int h=a.fontsize;
    int y=0;
#if 1
    if ((u==at_integrate) || (u==at_sum) ){ // Int
      int s=1;
      if (arg.type==_VECT)
	s=arg._VECTptr->size();
      else
	arg=vecteur(1,arg);
      // s==1 -> general case
      if ( (s==1) || (s==2) ){ // int f(x) dx and sum f(n) n
	vecteur v(Equation_subsizes(gen(*arg._VECTptr,_SEQ__VECT),a,windowhsize,contextptr));
	eqwdata vv(Equation_total_size(v[0]));
	if (s==1){
	  x=a.fontsize;
	  Equation_translate(v[0],x,0);
	  x += int(text_width(a.fontsize,(" dx")));
	}
	if (s==2){
	  if (u==at_integrate){
	    x=a.fontsize;
	    Equation_translate(v[0],x,0);
	    x += vv.dx+int(text_width(a.fontsize,(" d")));
	    Equation_vertical_adjust(vv.dy,vv.y,h,y);
	    vv=Equation_total_size(v[1]);
	    Equation_translate(v[1],x,0);
	    Equation_vertical_adjust(vv.dy,vv.y,h,y);
	  }
	  else {
	    Equation_vertical_adjust(vv.dy,vv.y,h,y);
	    eqwdata v1=Equation_total_size(v[1]);
	    x=max(a.fontsize,v1.dx)+2*a.fontsize/3; // var name size
	    Equation_translate(v[1],0,-v1.dy-v1.y);
	    Equation_vertical_adjust(v1.dy,-v1.dy,h,y);
	    Equation_translate(v[0],x,0);
	    x += vv.dx; // add function size
	  }
	}
	if (u==at_integrate){
	  x += vv.dx;
	  if (h==a.fontsize)
	    h+=2*a.fontsize/3;
	  if (y==0){
	    y=-2*a.fontsize/3;
	    h+=2*a.fontsize/3;
	  }
	}
	v.push_back(eqwdata(x,h,0,y,a,u,0));
	return gen(v,_SEQ__VECT);
      }
      if (s>=3){ // int _a^b f(x) dx
	vecteur & intarg=*arg._VECTptr;
	gen tmp_l,tmp_u,tmp_f,tmp_x;
	attributs aa(a);
	if (a.fontsize>=10)
	  aa.fontsize -= 2;
	tmp_f=Equation_compute_size(intarg[0],a,windowhsize,contextptr);
	tmp_x=Equation_compute_size(intarg[1],a,windowhsize,contextptr);
	tmp_l=Equation_compute_size(intarg[2],aa,windowhsize,contextptr);
	if (s==4)
	  tmp_u=Equation_compute_size(intarg[3],aa,windowhsize,contextptr);
	x=a.fontsize+(u==at_integrate?-2:+4);
	eqwdata vv(Equation_total_size(tmp_l));
	Equation_translate(tmp_l,x,-vv.y-vv.dy);
	vv=Equation_total_size(tmp_l);
	Equation_vertical_adjust(vv.dy,vv.y,h,y);
	int lx = vv.dx;
	if (s==4){
	  vv=Equation_total_size(tmp_u);
	  Equation_translate(tmp_u,x,a.fontsize-3-vv.y);
	  vv=Equation_total_size(tmp_u);
	  Equation_vertical_adjust(vv.dy,vv.y,h,y);
	}
	x += max(lx,vv.dx);
	Equation_translate(tmp_f,x,0);
	vv=Equation_total_size(tmp_f);
	Equation_vertical_adjust(vv.dy,vv.y,h,y);
	if (u==at_integrate){
	  x += vv.dx+int(text_width(a.fontsize,(" d")));
	  Equation_translate(tmp_x,x,0);
	  vv=Equation_total_size(tmp_x);
	  Equation_vertical_adjust(vv.dy,vv.y,h,y);
	  x += vv.dx;
	}
	else {
	  x += vv.dx;
	  Equation_vertical_adjust(vv.dy,vv.y,h,y);
	  vv=Equation_total_size(tmp_x);
	  x=max(x,vv.dx)+a.fontsize/3;
	  Equation_translate(tmp_x,0,-vv.dy-vv.y);
	  //Equation_translate(tmp_l,0,-1);	  
	  if (s==4) Equation_translate(tmp_u,-2,0);	  
	  Equation_vertical_adjust(vv.dy,-vv.dy,h,y);
	}
	vecteur res(makevecteur(tmp_f,tmp_x,tmp_l));
	if (s==4)
	  res.push_back(tmp_u);
	res.push_back(eqwdata(x,h,0,y,a,u,0));
	return gen(res,_SEQ__VECT);
      }
    }
    if (u==at_limit && arg.type==_VECT){ // limit
      vecteur limarg=*arg._VECTptr;
      int s=limarg.size();
      if (s==2 && limarg[1].is_symb_of_sommet(at_equal)){
	limarg.push_back(limarg[1]._SYMBptr->feuille[1]);
	limarg[1]=limarg[1]._SYMBptr->feuille[0];
	++s;
      }
      if (s>=3){
	gen tmp_l,tmp_f,tmp_x,tmp_dir;
	attributs aa(a);
	if (a.fontsize>=10)
	  aa.fontsize -= 2;
	tmp_f=Equation_compute_size(limarg[0],a,windowhsize,contextptr);
	tmp_x=Equation_compute_size(limarg[1],aa,windowhsize,contextptr);
	tmp_l=Equation_compute_size(limarg[2],aa,windowhsize,contextptr);
	if (s==4)
	  tmp_dir=Equation_compute_size(limarg[3],aa,windowhsize,contextptr);
	eqwdata vf(Equation_total_size(tmp_f));
	eqwdata vx(Equation_total_size(tmp_x));
	eqwdata vl(Equation_total_size(tmp_l));
	eqwdata vdir(Equation_total_size(tmp_dir));
	int sous=max(vx.dy,vl.dy);
	if (s==4)
	  Equation_translate(tmp_f,vx.dx+vl.dx+vdir.dx+a.fontsize+4,0);
	else
	  Equation_translate(tmp_f,vx.dx+vl.dx+a.fontsize+2,0);
	Equation_translate(tmp_x,0,-sous-vl.y);
	Equation_translate(tmp_l,vx.dx+a.fontsize+2,-sous-vl.y);
	if (s==4)
	  Equation_translate(tmp_dir,vx.dx+vl.dx+a.fontsize+4,-sous-vl.y);
	h=vf.dy;
	y=vf.y;
	vl=Equation_total_size(tmp_l);
	Equation_vertical_adjust(vl.dy,vl.y,h,y);
	vecteur res(makevecteur(tmp_f,tmp_x,tmp_l));
	if (s==4){
	  res.push_back(tmp_dir);
	  res.push_back(eqwdata(vf.dx+vx.dx+a.fontsize+4+vl.dx+vdir.dx,h,0,y,a,u,0));
	}
	else
	  res.push_back(eqwdata(vf.dx+vx.dx+a.fontsize+2+vl.dx,h,0,y,a,u,0));
	return gen(res,_SEQ__VECT);
      }
    }
#endif
    if ( (u==at_of || u==at_at) && arg.type==_VECT && arg._VECTptr->size()==2 ){
      // user function, function in 1st arg, arguments in 2nd arg
      gen varg1=Equation_compute_size(arg._VECTptr->front(),a,windowhsize,contextptr);
      eqwdata vv=Equation_total_size(varg1);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      gen arg2=arg._VECTptr->back();
      if (u==at_at && xcas_mode(contextptr)!=0){
	if (arg2.type==_VECT)
	  arg2=gen(addvecteur(*arg2._VECTptr,vecteur(arg2._VECTptr->size(),plus_one)),_SEQ__VECT);
	else
	  arg2=arg2+plus_one; 
      }
      gen varg2=Equation_compute_size(arg2,a,windowhsize,contextptr);
      Equation_translate(varg2,vv.dx+llp,0);
      vv=Equation_total_size(varg2);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      vecteur res(makevecteur(varg1,varg2));
      res.push_back(eqwdata(vv.dx+vv.x+lrp,h,0,y,a,u,0));
      return gen(res,_SEQ__VECT);
    }
    if (u==at_pow){ 
      // first arg not translated
      gen varg=Equation_compute_size(arg._VECTptr->front(),a,windowhsize,contextptr);
      eqwdata vv=Equation_total_size(varg);
      // 1/2 ->sqrt, otherwise as exponent
      if (arg._VECTptr->back()==plus_one_half){
	Equation_translate(varg,a.fontsize,0);
	vecteur res(1,varg);
	res.push_back(eqwdata(vv.dx+a.fontsize,vv.dy+4,vv.x,vv.y,a,at_sqrt,0));
	return gen(res,_SEQ__VECT);
      }
      bool needpar=vv.g.type==_FUNC || vv.g.is_symb_of_sommet(at_pow) || need_parenthesis(vv.g);
      if (needpar)
	x=llp;
      Equation_translate(varg,x,0);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      vecteur res(1,varg);
      // 2nd arg translated 
      if (needpar)
	x+=vv.dx+lrp;
      else
	x+=vv.dx+1;
      int arg1dy=vv.dy,arg1y=vv.y;
      if (a.fontsize>=16){
	attributs aa(a);
	aa.fontsize -= 2;
	varg=Equation_compute_size(arg._VECTptr->back(),aa,windowhsize,contextptr);
      }
      else
	varg=Equation_compute_size(arg._VECTptr->back(),a,windowhsize,contextptr);
      vv=Equation_total_size(varg);
      Equation_translate(varg,x,arg1y+(3*arg1dy)/4-vv.y);
      res.push_back(varg);
      vv=Equation_total_size(varg);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      x += vv.dx;
      res.push_back(eqwdata(x,h,0,y,a,u,0));
      return gen(res,_SEQ__VECT);
    }
    if (u==at_factorial){
      vecteur v;
      gen varg=Equation_compute_size(arg,a,windowhsize,contextptr);
      eqwdata vv=Equation_total_size(varg);
      bool paren=need_parenthesis(vv.g) || vv.g==at_prod || vv.g==at_division || vv.g==at_pow;
      if (paren)
	x+=llp;
      Equation_translate(varg,x,0);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      v.push_back(varg);
      x += vv.dx;
      if (paren)
	x+=lrp;
      varg=eqwdata(x+4,h,0,y,a,u,0);
      v.push_back(varg);
      return gen(v,_SEQ__VECT);
    }
    if (u==at_sto){ // A:=B, *it -> B
      gen varg=Equation_compute_size(arg._VECTptr->back(),a,windowhsize,contextptr);
      eqwdata vv=Equation_total_size(varg);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      Equation_translate(varg,x,0);
      vecteur v(2);
      v[1]=varg;
      x+=vv.dx;
      x+=ls+3;
      // first arg not translated
      varg=Equation_compute_size(arg._VECTptr->front(),a,windowhsize,contextptr);
      vv=Equation_total_size(varg);
      if (need_parenthesis(vv.g))
	x+=llp;
      Equation_translate(varg,x,0);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      v[0]=varg;
      x += vv.dx;
      if (need_parenthesis(vv.g))
	x+=lrp;
      v.push_back(eqwdata(x,h,0,y,a,u,0));
      return gen(v,_SEQ__VECT);
    }
    if (u==at_program && arg._VECTptr->back().type!=_VECT && !arg._VECTptr->back().is_symb_of_sommet(at_local) ){
      gen varg=Equation_compute_size(arg._VECTptr->front(),a,windowhsize,contextptr);
      eqwdata vv=Equation_total_size(varg);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      Equation_translate(varg,x,0);
      vecteur v(2);
      v[0]=varg;
      x+=vv.dx;
      x+=int(text_width(a.fontsize,("->")))+3;
      varg=Equation_compute_size(arg._VECTptr->back(),a,windowhsize,contextptr);
      vv=Equation_total_size(varg);
      Equation_translate(varg,x,0);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      v[1]=varg;
      x += vv.dx;
      v.push_back(eqwdata(x,h,0,y,a,u,0));
      return gen(v,_SEQ__VECT);      
    }
    bool binaryop= (u.ptr()->printsommet==&printsommetasoperator) || binary_op(u);
    if ( u!=at_sto && u.ptr()->printsommet!=NULL && !binaryop ){
      gen tmp=string2gen(g.print(contextptr),false);
      return Equation_compute_size(symbolic(at_expr,makesequence(tmp,xcas_mode(contextptr))),a,windowhsize,contextptr);
    }
    vecteur v;
    if (!binaryop || arg.type!=_VECT)
      v=Equation_subsizes(arg,a,windowhsize,contextptr);
    else
      v=Equation_subsizes(gen(*arg._VECTptr,_SEQ__VECT),a,windowhsize,contextptr);
    iterateur it=v.begin(),itend=v.end();
    if ( it==itend || (itend-it==1) ){ 
      gen gtmp;
      if (it==itend)
	gtmp=Equation_compute_size(gen(vecteur(0),_SEQ__VECT),a,windowhsize,contextptr);
      else
	gtmp=*it;
      // unary op, shift arg position horizontally
      eqwdata vv=Equation_total_size(gtmp);
      bool paren = u!=at_neg || (vv.g!=at_prod && need_parenthesis(vv.g)) ;
      x=ls+(paren?llp:0);
      gen tmp=gtmp; Equation_translate(tmp,x,0);
      x=x+vv.dx+(paren?lrp:0);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      return gen(makevecteur(tmp,eqwdata(x,h,0,y,a,u,0)),_EQW__VECT);
    }
    if (binaryop){ // op (default with par)
      int currenth=h,largeur=0;
      iterateur itprec=v.begin();
      h=0;
      if (u==at_plus){ // op without parenthesis
	if (it->type==_VECT && !it->_VECTptr->empty() && it->_VECTptr->back().type==_EQW && it->_VECTptr->back()._EQWptr->g==at_equal)
	  ;
	else {
	  llp=0;
	  lrp=0;
	}
      }
      for (;;){
	eqwdata vv=Equation_total_size(*it);
	if (need_parenthesis(vv.g))
	  x+=llp;
	if (u==at_plus && it!=v.begin() &&
	    ( 
	     (it->type==_VECT && it->_VECTptr->back().type==_EQW && it->_VECTptr->back()._EQWptr->g==at_neg) 
	     || 
	     ( it->type==_EQW && (is_integer(it->_EQWptr->g) || it->_EQWptr->g.type==_DOUBLE_) && is_strictly_positive(-it->_EQWptr->g,contextptr) ) 
	      ) 
	    )
	  x -= ls;
#if 0 //
	if (x>windowhsize-vv.dx && x>windowhsize/2 && (itend-it)*vv.dx>windowhsize/2){
	  largeur=max(x,largeur);
	  x=0;
	  if (need_parenthesis(vv.g))
	    x+=llp;
	  h+=currenth;
	  Equation_translate(*it,x,0);
	  for (iterateur kt=v.begin();kt!=itprec;++kt)
	    Equation_translate(*kt,0,currenth);
	  if (y){
	    for (iterateur kt=itprec;kt!=it;++kt)
	      Equation_translate(*kt,0,-y);
	  }
	  itprec=it;
	  currenth=vv.dy;
	  y=vv.y;
	}
	else
#endif
	  {
	    Equation_translate(*it,x,0);
	    vv=Equation_total_size(*it);
	    Equation_vertical_adjust(vv.dy,vv.y,currenth,y);
	  }
	x+=vv.dx;
	if (need_parenthesis(vv.g))
	  x+=lrp;
	++it;
	if (it==itend){
	  for (iterateur kt=v.begin();kt!=itprec;++kt)
	    Equation_translate(*kt,0,currenth+y);
	  h+=currenth;
	  v.push_back(eqwdata(max(x,largeur),h,0,y,a,u,0));
	  //cout << v << endl;
	  return gen(v,_SEQ__VECT);
	}
	x += ls+3;
      } 
    }
    // normal printing
    x=ls+llp;
    for (;;){
      eqwdata vv=Equation_total_size(*it);
      Equation_translate(*it,x,0);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      x+=vv.dx;
      ++it;
      if (it==itend){
	x+=lrp;
	v.push_back(eqwdata(x,h,0,y,a,u,0));
	return gen(v,_SEQ__VECT);
      }
      x+=lc;
    }
  }

  // windowhsize is used for g of type HIST__VECT (history) right justify answers
  // Returns either a eqwdata type object (terminal) or a vector 
  // (of subtype _EQW__VECT or _HIST__VECT)
  gen Equation_compute_size(const gen & g,const attributs & a,int windowhsize,GIAC_CONTEXT){
    /*****************
     *   FRACTIONS   *
     *****************/
    if (g.type==_FRAC){
      if (is_integer(g._FRACptr->num) && is_positive(-g._FRACptr->num,contextptr))
	return Equation_compute_size(symbolic(at_neg,fraction(-g._FRACptr->num,g._FRACptr->den)),a,windowhsize,contextptr);
      gen v1=Equation_compute_size(g._FRACptr->num,a,windowhsize,contextptr);
      eqwdata vv1=Equation_total_size(v1);
      gen v2=Equation_compute_size(g._FRACptr->den,a,windowhsize,contextptr);
      eqwdata vv2=Equation_total_size(v2);
      // Center the fraction
      int w1=vv1.dx,w2=vv2.dx;
      int w=max(w1,w2)+6;
      vecteur v(3);
      v[0]=v1; Equation_translate(v[0],(w-w1)/2,11-vv1.y);
      v[1]=v2; Equation_translate(v[1],(w-w2)/2,5-vv2.dy-vv2.y);
      v[2]=eqwdata(w,a.fontsize/2+vv1.dy+vv2.dy+1,0,(a.fontsize<=14?4:3)-vv2.dy,a,at_division,0);
      return gen(v,_SEQ__VECT);
    }
    /***************
     *   VECTORS   *
     ***************/
    if ( (g.type==_VECT) && !g._VECTptr->empty() ){
      vecteur v;
      const_iterateur it=g._VECTptr->begin(),itend=g._VECTptr->end();
      int x=0,y=0,h=a.fontsize; 
      /***************
       *   MATRICE   *
       ***************/
      bool gmat=ckmatrix(g);
      vector<int> V; int p=0;
      if (!gmat && is_mod_vecteur(*g._VECTptr,V,p) && p!=0){
	gen gm=makemodquoted(unmod(g),p);
	return Equation_compute_size(gm,a,windowhsize,contextptr);
      }
      vector< vector<int> > M; 
      if (gmat && is_mod_matrice(*g._VECTptr,M,p) && p!=0){
	gen gm=makemodquoted(unmod(g),p);
	return Equation_compute_size(gm,a,windowhsize,contextptr);
      }
      if (gmat && g.subtype!=_SEQ__VECT && g.subtype!=_SET__VECT && g.subtype!=_POLY1__VECT && g._VECTptr->front().subtype!=_SEQ__VECT){
	gen mkvect(at_makevector);
	mkvect.subtype=_SEQ__VECT;
	gen mkmat(at_makevector);
	mkmat.subtype=_MATRIX__VECT;
	int nrows,ncols;
	mdims(*g._VECTptr,nrows,ncols);
	if (ncols){
	  vecteur all_sizes;
	  all_sizes.reserve(nrows);
	  vector<int> row_heights(nrows),row_bases(nrows),col_widths(ncols);
	  // vertical gluing
	  for (int i=0;it!=itend;++it,++i){
	    gen tmpg=*it;
	    tmpg.subtype=_SEQ__VECT;
	    vecteur tmp(Equation_subsizes(tmpg,a,max(windowhsize/ncols-a.fontsize,230),contextptr));
	    int h=a.fontsize,y=0;
	    const_iterateur jt=tmp.begin(),jtend=tmp.end();
	    for (int j=0;jt!=jtend;++jt,++j){
	      eqwdata w(Equation_total_size(*jt));
	      Equation_vertical_adjust(w.dy,w.y,h,y);
	      col_widths[j]=max(col_widths[j],w.dx);
	    }
	    if (i)
	      row_heights[i]=row_heights[i-1]+h+a.fontsize/2;
	    else
	      row_heights[i]=h;
	    row_bases[i]=y;
	    all_sizes.push_back(tmp);
	  }
	  // accumulate col widths
	  col_widths.front() +=(3*a.fontsize)/2;
	  vector<int>::iterator iit=col_widths.begin()+1,iitend=col_widths.end();
	  for (;iit!=iitend;++iit)
	    *iit += *(iit-1)+a.fontsize;
	  // translate each cell
	  it=all_sizes.begin();
	  itend=all_sizes.end();
	  int h,y,prev_h=0;
	  for (int i=0;it!=itend;++it,++i){
	    h=row_heights[i];
	    y=row_bases[i];
	    iterateur jt=it->_VECTptr->begin(),jtend=it->_VECTptr->end();
	    for (int j=0;jt!=jtend;++jt,++j){
	      eqwdata w(Equation_total_size(*jt));
	      if (j)
		Equation_translate(*jt,col_widths[j-1]-w.x,-h-y);
	      else
		Equation_translate(*jt,-w.x+a.fontsize/2,-h-y);
	    }
	    it->_VECTptr->push_back(eqwdata(col_widths.back(),h-prev_h,0,-h,a,mkvect,0));
	    prev_h=h;
	  }
	  all_sizes.push_back(eqwdata(col_widths.back(),row_heights.back(),0,-row_heights.back(),a,mkmat,-row_heights.back()/2));
	  gen all_sizesg=all_sizes; Equation_translate(all_sizesg,0,row_heights.back()/2); return all_sizesg;
	}
      } // end matrices
      /*************************
       *   SEQUENCES/VECTORS   *
       *************************/
      // horizontal gluing
      if (g.subtype!=_PRINT__VECT) x += a.fontsize/2;
      int ncols=itend-it;
      //ncols=min(ncols,5);
      for (;it!=itend;++it){
	gen cur_size=Equation_compute_size(*it,a,
					   max(windowhsize/ncols-a.fontsize,
#ifdef IPAQ
					       200
#else
					       480
#endif
					       ),contextptr);
	eqwdata tmp=Equation_total_size(cur_size);
	Equation_translate(cur_size,x-tmp.x,0); v.push_back(cur_size);
	x=x+tmp.dx+((g.subtype==_PRINT__VECT)?2:a.fontsize);
	Equation_vertical_adjust(tmp.dy,tmp.y,h,y);
      }
      gen mkvect(at_makevector);
      if (g.subtype==_SEQ__VECT)
	mkvect=at_makesuite;
      else
	mkvect.subtype=g.subtype;
      v.push_back(eqwdata(x,h,0,y,a,mkvect,0));
      return gen(v,_EQW__VECT);
    } // end sequences
    if (g.type==_MOD){ 
      int x=0;
      int h=a.fontsize;
      int y=0;
      bool py=python_compat(contextptr);
      int modsize=int(text_width(a.fontsize,(py?" mod":"%")))+4;
      bool paren=is_positive(-*g._MODptr,contextptr);
      int llp=int(text_width(a.fontsize,("(")));
      int lrp=int(text_width(a.fontsize,(")")));
      gen varg1=Equation_compute_size(*g._MODptr,a,windowhsize,contextptr);
      if (paren) Equation_translate(varg1,llp,0);
      eqwdata vv=Equation_total_size(varg1);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      gen arg2=*(g._MODptr+1);
      gen varg2=Equation_compute_size(arg2,a,windowhsize,contextptr);
      if (paren)
	Equation_translate(varg2,vv.dx+modsize+lrp,0);
      else
	Equation_translate(varg2,vv.dx+modsize,0);
      vv=Equation_total_size(varg2);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      vecteur res(makevecteur(varg1,varg2));
      res.push_back(eqwdata(vv.dx+vv.x,h,0,y,a,at_normalmod,0));
      return gen(res,_SEQ__VECT);
    }
    if (g.type!=_SYMB){
      string s=g.type==_STRNG?*g._STRNGptr:g.print(contextptr);
      //if (g==cst_pi) s=char(129);
      if (s.size()>2000)
	s=s.substr(0,2000)+"...";
      int i=int(text_width(a.fontsize,(s.c_str())));
      gen tmp=eqwdata(i,a.fontsize,0,0,a,g);
      return tmp;
    }
    /**********************
     *  SYMBOLIC HANDLING *
     **********************/
    return Equation_compute_symb_size(g,a,windowhsize,contextptr);
    // return Equation_compute_symb_size(aplatir_fois_plus(g),a,windowhsize,contextptr);
    // aplatir_fois_plus is a problem for Equation_replace_selection
    // because it will modify the structure of the data
  }

  void Equation_draw(const eqwdata & e,int x,int y,int rightx,int lowery,Equation * eq,GIAC_CONTEXT){
    if ( (e.dx+e.x<x) || (e.x>rightx) || (e.y>y) || e.y+e.dy<lowery)
      ; // return; // nothing to draw, out of window
    gen gg=e.g;
    int fontsize=e.eqw_attributs.fontsize;
    int text_color=COLOR_BLACK;
    int background=COLOR_WHITE;
    string s=gg.type==_STRNG?*gg._STRNGptr:gg.print(contextptr);
    if (gg.type==_IDNT && !s.empty() && s[0]=='_')
      s=s.substr(1,s.size()-1);
    // if (gg==cst_pi){      s="p";      s[0]=(unsigned char)129;    }
    if (s.size()>2000)
      s=s.substr(0,2000)+"...";
    // cerr << s.size() << endl;
    text_print(fontsize,s.c_str(),eq->x()+e.x-x,eq->y()+y-e.y,text_color,background,e.selected?4:0);
    return;
  }

  inline void check_fl_rectf(int x,int y,int w,int h,int imin,int jmin,int di,int dj,int delta_i,int delta_j,int c){
    drawRectangle(x+delta_i,y+delta_j,w,h,c);
    //fl_rectf(x+delta_i,y+delta_j,w,h,c);
  }

  void Equation_draw(const gen & g,int x,int y,int rightx,int lowery,Equation * equat,GIAC_CONTEXT){
    int eqx=equat->x(),eqy=equat->y();
    if (g.type==_EQW){ // terminal
      eqwdata & e=*g._EQWptr;
      Equation_draw(e,x,y,rightx,lowery,equat,contextptr);
    }
    if (g.type!=_VECT)
      return;
    vecteur & v=*g._VECTptr;
    if (v.empty())
      return;
    gen tmp=v.back();
    if (tmp.type!=_EQW){
      cout << "EQW error:" << v << endl;
      return;
    }
    eqwdata & w=*tmp._EQWptr;
    if ( (w.dx+w.x-x<0) || (w.x>rightx) || (w.y>y) || (w.y+w.dy<lowery) )
      ; // return; // nothing to draw, out of window
    /*******************
     * draw the vector *
     *******************/
    // v is the vector, w the master operator eqwdata
    gen oper=w.g; 
    bool selected=w.selected ;
    int fontsize=w.eqw_attributs.fontsize;
    int background=w.eqw_attributs.background;
    int text_color=w.eqw_attributs.text_color;
    int mode=selected?4:0;
    int draw_line_color=selected?background:text_color;
    int x0=w.x;
    int y0=w.y; // lower coordinate of the master vector
    int y1=y0+w.dy; // upper coordinate of the master vector
    if (selected)
      drawRectangle(eqx+w.x-x,eqy+y-w.y-w.dy+1,w.dx,w.dy+1,text_color);
    // draw arguments of v
    const_iterateur it=v.begin(),itend=v.end()-1;
    if (oper==at_expr && v.size()==3){
      Equation_draw(*it,x,y,rightx,lowery,equat,contextptr);
      return;
    }
    for (;it!=itend;++it)
      Equation_draw(*it,x,y,rightx,lowery,equat,contextptr);
    if (oper==at_multistring)
      return;
    string s;
    if (oper.type==_FUNC){
      // catch here special cases user function, vect/matr, ^, int, sqrt, etc.
      unary_function_ptr & u=*oper._FUNCptr;
      if (u==at_at){ // draw brackets around 2nd arg
	gen arg2=v[1]; // 2nd arg of at_of, i.e. what's inside the parenth.
	eqwdata varg2=Equation_total_size(arg2);
	x0=varg2.x;
	y0=varg2.y;
	y1=y0+varg2.dy;
	fontsize=varg2.eqw_attributs.fontsize;
	if (x0<rightx)
	  text_print(fontsize,"[",eqx+x0-x-int(text_width(fontsize,("["))),eqy+y-varg2.baseline,text_color,background,mode);
	x0 += varg2.dx ;
	if (x0<rightx)
	  text_print(fontsize,"]",eqx+x0-x,eqy+y-varg2.baseline,text_color,background,mode);
	return;
      }
      if (u==at_of){ // do we need to draw some parenthesis?
	gen arg2=v[1]; // 2nd arg of at_of, i.e. what's inside the parenth.
	if (arg2.type!=_VECT || arg2._VECTptr->back().type !=_EQW || arg2._VECTptr->back()._EQWptr->g!=at_makesuite){ // Yes (if not _EQW it's a sequence with parent)
	  eqwdata varg2=Equation_total_size(arg2);
	  x0=varg2.x;
	  y0=varg2.y;
	  y1=y0+varg2.dy;
	  fontsize=varg2.eqw_attributs.fontsize;
	  int pfontsize=max(fontsize,(fontsize+(varg2.baseline-varg2.y))/2);
	  if (x0<rightx)
	    text_print(pfontsize,"(",eqx+x0-x-int(text_width(fontsize,("("))),eqy+y-varg2.baseline,text_color,background,mode);
	  x0 += varg2.dx ;
	  if (x0<rightx)
	    text_print(pfontsize,")",eqx+x0-x,eqy+y-varg2.baseline,text_color,background,mode);
	}
	return;
      }
      if (u==at_makesuite){
	bool paren=v.size()!=2; // Sequences with 1 arg don't show parenthesis
	int pfontsize=max(fontsize,(fontsize+(w.baseline-w.y))/2);
	if (paren && x0<rightx)
	  text_print(pfontsize,"(",eqx+x0-x-int(text_width(fontsize,("(")))/2,eqy+y-w.baseline,text_color,background,mode);
	x0 += w.dx;
	if (paren && x0<rightx)
	  text_print(pfontsize,")",eqx+x0-x-int(text_width(fontsize,("(")))/2,eqy+y-w.baseline,text_color,background,mode);
	// print commas between args
	it=v.begin(),itend=v.end()-2;
	for (;it!=itend;++it){
	  eqwdata varg2=Equation_total_size(*it);
	  fontsize=varg2.eqw_attributs.fontsize;
	  if (varg2.x+varg2.dx<rightx)
	    text_print(fontsize,",",eqx+varg2.x+varg2.dx-x+1,eqy+y-varg2.baseline,text_color,background,mode);
	}
	return;
      }
      if (u==at_makevector){ // draw [] delimiters for vector/matrices
	if (oper.subtype!=_SEQ__VECT && oper.subtype!=_PRINT__VECT){
	  int decal=1;
	  switch (oper.subtype){
	  case _MATRIX__VECT: decal=2; break;
	  case _SET__VECT: decal=4; break;
	  case _POLY1__VECT: decal=6; break;
	  }
	  if (eqx+x0-x+1>=0){
	    draw_line(eqx+x0-x+1,eqy+y-y0+1,eqx+x0-x+1,eqy+y-y1+1,draw_line_color);
	    draw_line(eqx+x0-x+decal,eqy+y-y0+1,eqx+x0-x+decal,eqy+y-y1+1,draw_line_color);
	    draw_line(eqx+x0-x+1,eqy+y-y0+1,eqx+x0-x+fontsize/4,eqy+y-y0+1,draw_line_color);
	    draw_line(eqx+x0-x+1,eqy+y-y1+1,eqx+x0-x+fontsize/4,eqy+y-y1+1,draw_line_color);
	  }
	  x0 += w.dx ;
	  if (eqx+x0-x-1<LCD_WIDTH_PX){
	    draw_line(eqx+x0-x-1,eqy+y-y0+1,eqx+x0-x-1,eqy+y-y1+1,draw_line_color);
	    draw_line(eqx+x0-x-decal,eqy+y-y0+1,eqx+x0-x-decal,eqy+y-y1+1,draw_line_color);
	    draw_line(eqx+x0-x-1,eqy+y-y0+1,eqx+x0-x-fontsize/4,eqy+y-y0+1,draw_line_color);
	    draw_line(eqx+x0-x-1,eqy+y-y1+1,eqx+x0-x-fontsize/4,eqy+y-y1+1,draw_line_color);
	  }
	} // end if oper.subtype!=SEQ__VECT
	if (oper.subtype!=_MATRIX__VECT && oper.subtype!=_PRINT__VECT){
	  // print commas between args
	  it=v.begin(),itend=v.end()-2;
	  for (;it!=itend;++it){
	    eqwdata varg2=Equation_total_size(*it);
	    fontsize=varg2.eqw_attributs.fontsize;
	    if (varg2.x+varg2.dx<rightx)
	      text_print(fontsize,",",eqx+varg2.x+varg2.dx-x+1,eqy+y-varg2.baseline,text_color,background,mode);
	  }
	}
	return;
      }
      int lpsize=int(text_width(fontsize,("(")));
      int rpsize=int(text_width(fontsize,(")")));
      eqwdata tmp=Equation_total_size(v.front()); // tmp= 1st arg eqwdata
      if (u==at_sto)
	tmp=Equation_total_size(v[1]);
      x0=w.x-x;
      y0=y-w.baseline;
      if (u==at_pow){
	if (!need_parenthesis(tmp.g)&& tmp.g!=at_pow && tmp.g!=at_prod && tmp.g!=at_division)
	  return;
	if (tmp.g==at_pow){
	  fontsize=tmp.eqw_attributs.fontsize+2;
	}
	if (tmp.x-lpsize<rightx)
	  text_print(fontsize,"(",eqx+tmp.x-x-lpsize,eqy+y-tmp.baseline,text_color,background,mode);
	if (tmp.x+tmp.dx<rightx)
	  text_print(fontsize,")",eqx+tmp.x+tmp.dx-x,eqy+y-tmp.baseline,text_color,background,mode);
	return;
      }
      if (u==at_program){
	if (tmp.x+tmp.dx<rightx)
	  text_print(fontsize,"->",eqx+tmp.x+tmp.dx-x,eqy+y-tmp.baseline,text_color,background,mode);
	return;
      }
#if 1
      if (u==at_sum){
	if (x0<rightx){
	  draw_line(eqx+x0,eqy+y0,eqx+x0+(2*fontsize)/3,eqy+y0,draw_line_color);
	  draw_line(eqx+x0,eqy+y0-fontsize,eqx+x0+(2*fontsize)/3,eqy+y0-fontsize,draw_line_color);
	  draw_line(eqx+x0,eqy+y0,eqx+x0+fontsize/2,eqy+y0-fontsize/2,draw_line_color);
	  draw_line(eqx+x0+fontsize/2,eqy+y0-fontsize/2,eqx+x0,eqy+y0-fontsize,draw_line_color);
	  if (v.size()>2){ // draw the =
	    eqwdata ptmp=Equation_total_size(v[1]);
	    if (ptmp.x+ptmp.dx<rightx)
	      text_print(fontsize,"=",eqx+ptmp.x+ptmp.dx-x-2,eqy+y-ptmp.baseline,text_color,background,mode);
	  }
	}
	return;
      }
#endif
      if (u==at_abs){
	y0 =1+y-w.y;
	int h=w.dy;
	if (x0<rightx){
	  draw_line(eqx+x0+2,eqy+y0-1,eqx+x0+2,eqy+y0-h+3,draw_line_color);
	  draw_line(eqx+x0+1,eqy+y0-1,eqx+x0+1,eqy+y0-h+3,draw_line_color);
	  draw_line(eqx+x0+w.dx-1,eqy+y0-1,eqx+x0+w.dx-1,eqy+y0-h+3,draw_line_color);
	  draw_line(eqx+x0+w.dx,eqy+y0-1,eqx+x0+w.dx,eqy+y0-h+3,draw_line_color);
	}
	return;
      }
      if (u==at_sqrt){
	y0 =1+y-w.y;
	int h=w.dy;
	if (x0<rightx){
	  draw_line(eqx+x0+2,eqy+y0-h/2,eqx+x0+fontsize/2,eqy+y0-1,draw_line_color);
	  draw_line(eqx+x0+fontsize/2,eqy+y0-1,eqx+x0+fontsize,eqy+y0-h+3,draw_line_color);
	  draw_line(eqx+x0+fontsize,eqy+y0-h+3,eqx+x0+w.dx-1,eqy+y0-h+3,draw_line_color);
	  ++y0;
	  draw_line(eqx+x0+2,eqy+y0-h/2,eqx+x0+fontsize/2,eqy+y0-1,draw_line_color);
	  draw_line(eqx+x0+fontsize/2,eqy+y0-1,eqx+x0+fontsize,eqy+y0-h+3,draw_line_color);
	  draw_line(eqx+x0+fontsize,eqy+y0-h+3,eqx+x0+w.dx-1,eqy+y0-h+3,draw_line_color);
	}
	return;
      }
      if (u==at_factorial){
	text_print(fontsize,"!",eqx+w.x+w.dx-4-x,eqy+y-w.baseline,text_color,background,mode);
	if (!need_parenthesis(tmp.g)
	    && tmp.g!=at_pow && tmp.g!=at_prod && tmp.g!=at_division
	    )
	  return;
	if (tmp.x-lpsize<rightx)
	  text_print(fontsize,"(",eqx+tmp.x-x-lpsize,eqy+y-tmp.baseline,text_color,background,mode);
	if (tmp.x+tmp.dx<rightx)
	  text_print(fontsize,")",eqx+tmp.x+tmp.dx-x,eqy+y-tmp.baseline,text_color,background,mode);
	return;
      }
#if 1
      if (u==at_integrate){
	x0+=2;
	y0+=fontsize/2;
	if (x0<rightx){
	  fl_arc(eqx+x0,eqy+y0,fontsize/3,fontsize/3,180,360,draw_line_color);
	  draw_line(eqx+x0+fontsize/3,eqy+y0,eqx+x0+fontsize/3,eqy+y0-2*fontsize+4,draw_line_color);
	  fl_arc(eqx+x0+fontsize/3,eqy+y0-2*fontsize+3,fontsize/3,fontsize/3,0,180,draw_line_color);
	}
	if (v.size()!=2){ // if arg has size > 1 draw the d
	  eqwdata ptmp=Equation_total_size(v[1]);
	  if (ptmp.x<rightx)
	    text_print(fontsize," d",eqx+ptmp.x-x-int(text_width(fontsize,(" d"))),eqy+y-ptmp.baseline,text_color,background,mode);
	}
	else {
	  eqwdata ptmp=Equation_total_size(v[0]);
	  if (ptmp.x+ptmp.dx<rightx)
	    text_print(fontsize," dx",eqx+ptmp.x+ptmp.dx-x,eqy+y-ptmp.baseline,text_color,background,mode);
	}
	return;
      }
#endif
      if (u==at_division){
	if (x0<rightx){
	  int yy=eqy+y0-8;
	  draw_line(eqx+x0+2,yy,eqx+x0+w.dx-2,yy,draw_line_color);
	  ++yy;
	  draw_line(eqx+x0+2,yy,eqx+x0+w.dx-2,yy,draw_line_color);
	}
	return;
      }
#if 1
      if (u==at_limit && v.size()>=4){
	if (x0<rightx)
	  text_print(fontsize,"lim",eqx+w.x-x,eqy+y-w.baseline,text_color,background,mode);
	gen arg2=v[1]; // 2nd arg of limit, i.e. the variable
	if (arg2.type==_EQW){ 
	  eqwdata & varg2=*arg2._EQWptr;
	  if (varg2.x+varg2.dx+2<rightx)
	    text_print(fontsize,"\x1e",eqx+varg2.x+varg2.dx+2-x,eqy+y-varg2.y,text_color,background,mode);
	}
	if (v.size()>=5){
	  arg2=v[2]; // 3rd arg of lim, the point, draw a comma after if dir.
	  if (arg2.type==_EQW){ 
	    eqwdata & varg2=*arg2._EQWptr;
	    if (varg2.x+varg2.dx<rightx)
	      text_print(fontsize,",",eqx+varg2.x+varg2.dx-x,eqy+y-varg2.baseline,text_color,background,mode);
	  }
	}
	return;
      } // limit
#endif
      bool parenthesis=true;
      string opstring(",");
      if (u.ptr()->printsommet==&printsommetasoperator || binary_op(u) ){
	if (u==at_normalmod && python_compat(contextptr))
	  opstring=" mod";
	else
	  opstring=u.ptr()->s;
      }
      else {
	if (u==at_sto)
	  opstring=":=";
	parenthesis=false;
      }
      // int yy=y0; // y0 is the lower coordinate of the whole eqwdata
      // int opsize=int(text_width(fontsize,(opstring.c_str())))+3;
      it=v.begin();
      itend=v.end()-1;
      // Reminder: here tmp is the 1st arg eqwdata, w the whole eqwdata
      if ( (itend-it==1) && ( (u==at_neg) 
			      || (u==at_plus) // uncommented for +infinity
			      ) ){ 
	if ( (u==at_neg &&need_parenthesis(tmp.g) && tmp.g!=at_prod)){
	  if (tmp.x-lpsize<rightx)
	    text_print(fontsize,"(",eqx+tmp.x-x-lpsize,eqy+y-tmp.baseline,text_color,background,mode);
	  if (tmp.x+tmp.dx<rightx)
	    text_print(fontsize,")",eqx+tmp.x-x+tmp.dx,eqy+y-tmp.baseline,text_color,background,mode);
	}
	if (w.x<rightx){
	  text_print(fontsize,u.ptr()->s,eqx+w.x-x,eqy+y-w.baseline,text_color,background,mode);
	}
	return;
      }
      // write first open parenthesis
      if (u==at_plus && tmp.g!=at_equal)
	parenthesis=false;
      else {
	if (parenthesis && need_parenthesis(tmp.g)){
	  if (w.x<rightx){
	    int pfontsize=max(fontsize,(fontsize+(tmp.baseline-tmp.y))/2);
	    text_print(pfontsize,"(",eqx+w.x-x,eqy+y-tmp.baseline,text_color,background,mode);
	  }
	}
      }
      for (;;){
	// write close parenthesis at end
	int xx=tmp.dx+tmp.x-x;
	if (parenthesis && need_parenthesis(tmp.g)){
	  if (xx<rightx){
	    int pfontsize=min(max(fontsize,(fontsize+(tmp.baseline-tmp.y))/2),fontsize*2);
	    int deltapary=(2*(pfontsize-fontsize))/3;
	    text_print(pfontsize,")",eqx+xx,eqy+y-tmp.baseline+deltapary,text_color,background,mode);
	  }
	  xx +=rpsize;
	}
	++it;
	if (it==itend){
	  if (u.ptr()->printsommet==&printsommetasoperator || u==at_sto || binary_op(u))
	    return;
	  else
	    break;
	}
	// write operator
	if (u==at_prod){
	  // text_print(fontsize,".",eqx+xx+3,eqy+y-tmp.baseline-fontsize/3);
	  text_print(fontsize,opstring.c_str(),eqx+xx+1,eqy+y-tmp.baseline,text_color,background,mode);
	}
	else {
	  gen tmpgen;
	  if (u==at_plus && ( 
			     (it->type==_VECT && it->_VECTptr->back().type==_EQW && it->_VECTptr->back()._EQWptr->g==at_neg) 
			     || 
			     ( it->type==_EQW && (is_integer(it->_EQWptr->g) || it->_EQWptr->g.type==_DOUBLE_) && is_strictly_positive(-it->_EQWptr->g,contextptr) ) 
			      )
	      )
	    ;
	  else {
	    if (xx+1<rightx)
	      // fl_draw(opstring.c_str(),xx+1,y-tmp.y-tmp.dy/2+fontsize/2);
	      text_print(fontsize,opstring.c_str(),eqx+xx+1,eqy+y-tmp.baseline,text_color,background,mode);
	  }
	}
	// write right parent, update tmp
	tmp=Equation_total_size(*it);
	if (parenthesis && (need_parenthesis(tmp.g)) ){
	  if (tmp.x-lpsize<rightx){
	    int pfontsize=min(max(fontsize,(fontsize+(tmp.baseline-tmp.y))/2),fontsize*2);
	    int deltapary=(2*(pfontsize-fontsize))/3;
	    text_print(pfontsize,"(",eqx+tmp.x-pfontsize*lpsize/fontsize-x,eqy+y-tmp.baseline+deltapary,text_color,background,mode);
	  }
	}
      } // end for (;;)
      if (w.x<rightx){
	s = u.ptr()->s;
	s += '(';
	text_print(fontsize,s.c_str(),eqx+w.x-x,eqy+y-w.baseline,text_color,background,mode);
      }
      if (w.x+w.dx-rpsize<rightx)
	text_print(fontsize,")",eqx+w.x+w.dx-x-rpsize+2,eqy+y-w.baseline,text_color,background,mode);
      return;
    }
    s=oper.print(contextptr);
    if (w.x<rightx){
      text_print(fontsize,s.c_str(),eqx+w.x-x,eqy+y-w.baseline,text_color,background,mode);
    }
  }

  Equation::Equation(int x_, int y_, const gen & g,const giac::context * cptr){
    _x=x_;
    _y=y_;
    attr=attributs(18,COLOR_WHITE,COLOR_BLACK);
    contextptr=cptr;
    if (taille(g,max_prettyprint_equation)<max_prettyprint_equation)
      data=Equation_compute_size(g,attr,LCD_WIDTH_PX,contextptr);
    else
      data=Equation_compute_size(string2gen("Object_too_large",false),attr,LCD_WIDTH_PX,contextptr);
    undodata=Equation_copy(data);
  }

  void replace_selection(Equation & eq,const gen & tmp,gen * gsel,const vector<int> * gotoptr,GIAC_CONTEXT){
    int xleft,ytop,xright,ybottom,gselpos; gen *gselparent;
    vector<int> goto_sel;
    eq.undodata=Equation_copy(eq.data);
    if (gotoptr==0){
      if (xcas::Equation_adjust_xy(eq.data,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos,&goto_sel) && gsel)
	gotoptr=&goto_sel;
      else
	return;
    }
    *gsel=xcas::Equation_compute_size(tmp,eq.attr,LCD_WIDTH_PX,contextptr);
    gen value;
    xcas::do_select(eq.data,true,value);
    if (value.type==_EQW)
      eq.data=xcas::Equation_compute_size(value._EQWptr->g,eq.attr,LCD_WIDTH_PX,contextptr);
    //cout << "new value " << value << " " << eq.data << " " << *gotoptr << endl;
    xcas::Equation_select(eq.data,false);
    gen * gptr=&eq.data;
    for (int i=gotoptr->size()-1;i>=0;--i){
      int pos=(*gotoptr)[i];
      if (gptr->type==_VECT &&gptr->_VECTptr->size()>pos)
	gptr=&(*gptr->_VECTptr)[pos];
    }
    xcas::Equation_select(*gptr,true);
    //cout << "new sel " << *gptr << endl;
  }

  void display(Equation & eq,int x,int y,GIAC_CONTEXT){
    // Equation_draw(eq.data,x,y,LCD_WIDTH_PX,0,&eq,contextptr);
    int xleft,ytop,xright,ybottom,gselpos; gen * gsel,*gselparent;
    eqwdata eqdata=Equation_total_size(eq.data);
    if ( (eqdata.dx>LCD_WIDTH_PX || eqdata.dy>LCD_HEIGHT_PX-STATUS_AREA_PX) && Equation_adjust_xy(eq.data,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos)){
      if (x<xleft){
	if (x+LCD_WIDTH_PX<xright)
	  x=giacmin(xleft,xright-LCD_WIDTH_PX);
      }
      if (x>=xleft && x+LCD_WIDTH_PX>=xright){
	if (xright-x<LCD_WIDTH_PX)
	  x=giacmax(xright-LCD_WIDTH_PX,0);
      }
#if 0
      cout << "avant " << y << " " << ytop << " " << ybottom << endl;
      if (y<ytop){
	if (y+LCD_HEIGHT_PX<ybottom)
	  y=giacmin(ytop,ybottom-LCD_HEIGHT_PX);
      }
      if (y>=ytop && y+LCD_HEIGHT_PX>=ybottom){
	if (ybottom-y<LCD_HEIGHT_PX)
	  y=giacmax(ybottom-LCD_HEIGHT_PX,0);
      }
      cout << "apres " << y << " " << ytop << " " << ybottom << endl;
#endif
    }
    int save_ymin_clip=clip_ymin;
    clip_ymin=STATUS_AREA_PX;
    Equation_draw(eq.data,x,y,RAND_MAX,0,&eq,contextptr);
    clip_ymin=save_ymin_clip;
  }
  
  /* ******************* *
   *      GRAPH          *
   * ******************* *
   */
#if 1

  double find_tick(double dx){
    double d=std::pow(10.0,std::floor(std::log10(absdouble(dx))));
    if (dx<2*d)
      d=d/5;
    else {
      if (dx<5*d)
	d=d/2;
    }
    return d;
  }

  Graph2d::Graph2d(const giac::gen & g_,const giac::context * cptr):window_xmin(gnuplot_xmin),window_xmax(gnuplot_xmax),window_ymin(gnuplot_ymin),window_ymax(gnuplot_ymax),g(g_),display_mode(0x45),show_axes(1),show_names(1),labelsize(16),contextptr(cptr) {
    update();
    autoscale();
  }
  
  void Graph2d::zoomx(double d,bool round){
    double x_center=(window_xmin+window_xmax)/2;
    double dx=(window_xmax-window_xmin);
    if (dx==0)
      dx=gnuplot_xmax-gnuplot_xmin;
    dx *= d/2;
    x_tick = find_tick(dx);
    window_xmin = x_center - dx;
    if (round) 
      window_xmin=int( window_xmin/x_tick -1)*x_tick;
    window_xmax = x_center + dx;
    if (round)
      window_xmax=int( window_xmax/x_tick +1)*x_tick;
    update();
  }

  void Graph2d::zoomy(double d,bool round){
    double y_center=(window_ymin+window_ymax)/2;
    double dy=(window_ymax-window_ymin);
    if (dy==0)
      dy=gnuplot_ymax-gnuplot_ymin;
    dy *= d/2;
    y_tick = find_tick(dy);
    window_ymin = y_center - dy;
    if (round)
      window_ymin=int( window_ymin/y_tick -1)*y_tick;
    window_ymax = y_center + dy;
    if (round)
      window_ymax=int( window_ymax/y_tick +1)*y_tick;
    update();
  }

  void Graph2d::zoom(double d){ 
    zoomx(d);
    zoomy(d);
  }

  void Graph2d::autoscale(bool fullview){
    // Find the largest and lowest x/y/z in objects (except lines/plans)
    vector<double> vx,vy,vz;
    int s;
    bool ortho=autoscaleg(g,vx,vy,vz,contextptr);
    autoscaleminmax(vx,window_xmin,window_xmax,fullview);
    zoomx(1.0);
    autoscaleminmax(vy,window_ymin,window_ymax,fullview);
    zoomy(1.0);
    bool do_ortho=ortho;
    if (!do_ortho){
      double w=LCD_WIDTH_PX;
      double h=LCD_HEIGHT_PX-STATUS_AREA_PX;
      double window_w=window_xmax-window_xmin,window_h=window_ymax-window_ymin;
      double tst=h/w*window_w/window_h;
      if (tst>0.7 && tst<1.4)
	do_ortho=true;
    }
    if (do_ortho )
      orthonormalize();
    y_tick=find_tick(window_ymax-window_ymin);
    update();
  }

  void Graph2d::orthonormalize(){ 
    // Center of the directions, orthonormalize
    double w=LCD_WIDTH_PX;
    double h=LCD_HEIGHT_PX-STATUS_AREA_PX;
    double window_w=window_xmax-window_xmin,window_h=window_ymax-window_ymin;
    double window_hsize=h/w*window_w;
    if (window_h > window_hsize*1.01){ // enlarge horizontally
      double window_xcenter=(window_xmin+window_xmax)/2;
      double window_wsize=w/h*window_h;
      window_xmin=window_xcenter-window_wsize/2;
      window_xmax=window_xcenter+window_wsize/2;
    }
    if (window_h < window_hsize*0.99) { // enlarge vertically
      double window_ycenter=(window_ymin+window_ymax)/2;
      window_ymin=window_ycenter-window_hsize/2;
      window_ymax=window_ycenter+window_hsize/2;
    }
    x_tick=find_tick(window_xmax-window_xmin);
    y_tick=find_tick(window_ymax-window_ymin);
    update();
  }

  void Graph2d::update(){
    x_scale=LCD_WIDTH_PX/(window_xmax-window_xmin);    
    y_scale=(LCD_HEIGHT_PX-STATUS_AREA_PX)/(window_ymax-window_ymin);    
  }

  bool Graph2d::findij(const gen & e0,double x_scale,double y_scale,double & i0,double & j0,GIAC_CONTEXT) const {
    gen e,f0,f1;
    evalfdouble2reim(e0,e,f0,f1,contextptr);
    if ((f0.type==_DOUBLE_) && (f1.type==_DOUBLE_)){
      if (display_mode & 0x400){
	if (f0._DOUBLE_val<=0)
	  return false;
	f0=std::log10(f0._DOUBLE_val);
      }
      i0=(f0._DOUBLE_val-window_xmin)*x_scale;
      if (display_mode & 0x800){
	if (f1._DOUBLE_val<=0)
	  return false;
	f1=std::log10(f1._DOUBLE_val);
      }
      j0=(window_ymax-f1._DOUBLE_val)*y_scale;
      return true;
    }
    // cerr << "Invalid drawing data" << endl;
    return false;
  }

  inline void swapint(int & i0,int & i1){
    int tmp=i0;
    i0=i1;
    i1=tmp;
  }

  void check_fl_draw(int fontsize,const char * ch,int i0,int j0,int imin,int jmin,int di,int dj,int delta_i,int delta_j,int c){
    /* int n=fl_size();
       if (j0>=jmin-n && j0<=jmin+dj+n) */
    // cerr << i0 << " " << j0 << endl;
    if (strlen(ch)>200)
      text_print(fontsize,"String too long",i0+delta_i,j0+delta_j,c);
    else
      text_print(fontsize,ch,i0+delta_i,j0+delta_j,c);
  }

  inline void check_fl_point(int i0,int j0,int imin,int jmin,int di,int dj,int delta_i,int delta_j,int c){
    /* if (i0>=imin && i0<=imin+di && j0>=jmin && j0<=jmin+dj) */
    numworks_giac_set_pixel(i0+delta_i,j0+delta_j,c);
  }

  inline void fl_line(int x0,int y0,int x1,int y1,int c){
    draw_line(x0,y0,x1,y1,c);
  }

  inline void fl_polygon(int x0,int y0,int x1,int y1,int x2,int y2,int c){
    draw_line(x0,y0,x1,y1,c);
    draw_line(x1,y1,x2,y2,c);
  }

  inline void check_fl_line(int i0,int j0,int i1,int j1,int imin,int jmin,int di,int dj,int delta_i,int delta_j,int c){
    fl_line(i0+delta_i,j0+delta_j,i1+delta_i,j1+delta_j,c);
  }

  int logplot_points=20;

  void checklog_fl_line(double i0,double j0,double i1,double j1,double deltax,double deltay,bool logx,bool logy,double window_xmin,double x_scale,double window_ymax,double y_scale,int c){
    if (!logx && !logy){
      fl_line(round(i0+deltax),round(j0+deltay),round(i1+deltax),round(j1+deltay),c);
      return;
    }
  }

  void find_dxdy(const string & legendes,int labelpos,int labelsize,int & dx,int & dy){
    int l=text_width(labelsize,legendes.c_str());
    dx=3;
    dy=1;
    switch (labelpos){
    case 1:
      dx=-l-3;
      break;
    case 2:
      dx=-l-3;
      dy=labelsize-2;
      break;
    case 3:
      dy=labelsize-2;
      break;
    }
    dy += labelsize;
  }

  void draw_legende(const vecteur & f,int i0,int j0,int labelpos,const Graph2d * iptr,int clip_x,int clip_y,int clip_w,int clip_h,int deltax,int deltay,int c,GIAC_CONTEXT){
    if (f.empty() ||!iptr->show_names )
      return;
    string legendes;
    if (f[0].is_symb_of_sommet(at_curve)){
      gen & f0=f[0]._SYMBptr->feuille;
      if (f0.type==_VECT && !f0._VECTptr->empty()){
	gen & f1 = f0._VECTptr->front();
	if (f1.type==_VECT && f1._VECTptr->size()>4 && (!is_zero((*f1._VECTptr)[4]) || (iptr->show_names & 2)) ){
	  gen legende=f1._VECTptr->front();
	  gen var=(*f1._VECTptr)[1];
	  gen r=re(legende,contextptr),i=im(legende,contextptr),a,b;
	  if (var.type==_IDNT && is_linear_wrt(r,*var._IDNTptr,a,b,contextptr)){
	    i=subst(i,var,(var-b)/a,false,contextptr);
	    legendes=i.print(contextptr);
	  }
	  else
	    legendes=r.print(contextptr)+","+i.print(contextptr);
	  if (legendes.size()>18){
	    if (legendes.size()>30)
	      legendes="";
	    else
	      legendes=legendes.substr(0,16)+"...";
	  }
	}
      }
    }
    if (f.size()>2)
      legendes=gen2string(f[2])+(legendes.empty()?"":":")+legendes;
    if (legendes.empty())
      return;
    int fontsize=iptr->labelsize;
    int dx=3,dy=1;
    find_dxdy(legendes,labelpos,fontsize,dx,dy);
    check_fl_draw(fontsize,legendes.c_str(),i0+dx,j0+dy,clip_x,clip_y,clip_w,clip_h,deltax,deltay,c);
  }

  void petite_fleche(double i1,double j1,double dx,double dy,int deltax,int deltay,int width,int c){
    double dxy=std::sqrt(dx*dx+dy*dy);
    if (dxy){
      dxy/=max(2,min(5,int(dxy/10)))+width;
      dx/=dxy;
      dy/=dxy;
      double dxp=-dy,dyp=dx; // perpendicular
      dx*=std::sqrt(3.0);
      dy*=sqrt(3.0);
      fl_polygon(round(i1)+deltax,round(j1)+deltay,round(i1+dx+dxp)+deltax,round(j1+dy+dyp)+deltay,round(i1+dx-dxp)+deltax,round(j1+dy-dyp)+deltay,c);
    }
  }

  void fltk_point(int deltax,int deltay,int i0,int j0,int epaisseur_point,int type_point,int c){
    switch (type_point){
    case 1: // losange
      fl_line(deltax+i0-epaisseur_point,deltay+j0,deltax+i0,deltay+j0-epaisseur_point,c);
      fl_line(deltax+i0,deltay+j0-epaisseur_point,deltax+i0+epaisseur_point,deltay+j0,c);
      fl_line(deltax+i0-epaisseur_point,deltay+j0,deltax+i0,deltay+j0+epaisseur_point,c);
      fl_line(deltax+i0,deltay+j0+epaisseur_point,deltax+i0+epaisseur_point,deltay+j0,c);
      break;
    case 2: // croix verticale
      fl_line(deltax+i0,deltay+j0-epaisseur_point,deltax+i0,deltay+j0+epaisseur_point,c);
      fl_line(deltax+i0-epaisseur_point,deltay+j0,deltax+i0+epaisseur_point,deltay+j0,c);
      break;
    case 3: // carre
      fl_line(deltax+i0-epaisseur_point,deltay+j0-epaisseur_point,deltax+i0-epaisseur_point,deltay+j0+epaisseur_point,c);
      fl_line(deltax+i0+epaisseur_point,deltay+j0-epaisseur_point,deltax+i0+epaisseur_point,deltay+j0+epaisseur_point,c);
      fl_line(deltax+i0-epaisseur_point,deltay+j0-epaisseur_point,deltax+i0+epaisseur_point,deltay+j0-epaisseur_point,c);
      fl_line(deltax+i0-epaisseur_point,deltay+j0+epaisseur_point,deltax+i0+epaisseur_point,deltay+j0+epaisseur_point,c);
      break;
    case 5: // triangle
      fl_line(deltax+i0-epaisseur_point,deltay+j0,deltax+i0,deltay+j0-epaisseur_point,c);
      fl_line(deltax+i0,deltay+j0-epaisseur_point,deltax+i0+epaisseur_point,deltay+j0,c);
      fl_line(deltax+i0-epaisseur_point,deltay+j0,deltax+i0+epaisseur_point,deltay+j0,c);
      break;
    case 7: // point
      if (epaisseur_point>2)
	fl_arc(deltax+i0-(epaisseur_point-1),deltay+j0-(epaisseur_point-1),2*(epaisseur_point-1),2*(epaisseur_point-1),0,360,c);
      else
	fl_line(deltax+i0,deltay+j0,deltax+i0+1,deltay+j0,c);
      break;
    case 6: // etoile
      fl_line(deltax+i0-epaisseur_point,deltay+j0,deltax+i0+epaisseur_point,deltay+j0,c);
      // no break to add the following lines
    case 0: // 0 croix diagonale
      fl_line(deltax+i0-epaisseur_point,deltay+j0-epaisseur_point,deltax+i0+epaisseur_point,deltay+j0+epaisseur_point,c);
      fl_line(deltax+i0-epaisseur_point,deltay+j0+epaisseur_point,deltax+i0+epaisseur_point,deltay+j0-epaisseur_point,c);
      break;
    default: // 4 nothing drawn
      break;
    }
  }

  int horiz_or_vert(const_iterateur jt,GIAC_CONTEXT){
    gen tmp(*(jt+1)-*jt),r,i;
    reim(tmp,r,i,contextptr);
    if (is_zero(r,contextptr)) return 1;
    if (is_zero(i,contextptr)) return 2;
    return 0;
  }

  void fltk_draw(Graph2d & Mon_image,const gen & g,double x_scale,double y_scale,int clip_x,int clip_y,int clip_w,int clip_h,GIAC_CONTEXT){
    int deltax=0,deltay=STATUS_AREA_PX,fontsize=Mon_image.labelsize;
    if (g.type==_VECT){
      const vecteur & v=*g._VECTptr;
      const_iterateur it=v.begin(),itend=v.end();
      for (;it!=itend;++it)
	fltk_draw(Mon_image,*it,x_scale,y_scale,clip_x,clip_y,clip_w,clip_h,contextptr);
    }
    if (g.type!=_SYMB)
      return;
    unary_function_ptr s=g._SYMBptr->sommet;
    if (g._SYMBptr->feuille.type!=_VECT)
      return;
    vecteur f=*g._SYMBptr->feuille._VECTptr;
    int mxw=LCD_WIDTH_PX,myw=LCD_HEIGHT_PX-STATUS_AREA_PX;
    double i0,j0,i0save,j0save,i1,j1;
    int fs=f.size();
    if ((fs==4) && (s==at_parameter)){
      return ;
    }
    string the_legend;
    vecteur style(get_style(f,the_legend));
    int styles=style.size();
    // color
    int ensemble_attributs = style.front().val;
    bool hidden_name = false;
    if (style.front().type==_ZINT){
      ensemble_attributs = mpz_get_si(*style.front()._ZINTptr);
      hidden_name=true;
    }
    else
      hidden_name=ensemble_attributs<0;
    int width           =(ensemble_attributs & 0x00070000) >> 16; // 3 bits
    int epaisseur_point =(ensemble_attributs & 0x00380000) >> 19; // 3 bits
    int type_line       =(ensemble_attributs & 0x01c00000) >> 22; // 3 bits
    if (type_line>4)
      type_line=(type_line-4)<<8;
    int type_point      =(ensemble_attributs & 0x0e000000) >> 25; // 3 bits
    int labelpos        =(ensemble_attributs & 0x30000000) >> 28; // 2 bits
    bool fill_polygon   =(ensemble_attributs & 0x40000000) >> 30;
    int couleur         =(ensemble_attributs & 0x0007ffff);
    epaisseur_point += 2;
    if (s==at_pnt){ 
      // f[0]=complex pnt or vector of complex pnts or symbolic
      // f[1] -> style 
      // f[2] optional=label
      gen point=f[0];
      if (point.type==_VECT && point.subtype==_POINT__VECT)
	return;
      if ( (f[0].type==_SYMB) && (f[0]._SYMBptr->sommet==at_curve) && (f[0]._SYMBptr->feuille.type==_VECT) && (f[0]._SYMBptr->feuille._VECTptr->size()) ){
	// Mon_image.show_mouse_on_object=false;
	point=f[0]._SYMBptr->feuille._VECTptr->back();
	if (type_line>=4 && point.type==_VECT && point._VECTptr->size()>2){
	  vecteur v=*point._VECTptr;
	  int vs=v.size()/2; // 3 -> 1
	  if (Mon_image.findij(v[vs],x_scale,y_scale,i0,j0,contextptr) && Mon_image.findij(v[vs+1],x_scale,y_scale,i1,j1,contextptr)){
	    bool logx=Mon_image.display_mode & 0x400,logy=Mon_image.display_mode & 0x800;
	    checklog_fl_line(i0,j0,i1,j1,deltax,deltay,logx,logy,Mon_image.window_xmin,x_scale,Mon_image.window_ymax,y_scale,couleur);
	    double dx=i0-i1,dy=j0-j1;
	    petite_fleche(i1,j1,dx,dy,deltax,deltay,width+3,couleur);
	  }
	}
      }
      if (is_undef(point))
	return;
      // fl_line_style(type_line,width+1,0); 
      if (point.type==_SYMB) {
	if (point._SYMBptr->sommet==at_cercle){
	  vecteur v=*point._SYMBptr->feuille._VECTptr;
	  gen diametre=remove_at_pnt(v[0]);
	  gen e1=diametre._VECTptr->front().evalf_double(1,contextptr),e2=diametre._VECTptr->back().evalf_double(1,contextptr);
	  gen centre=rdiv(e1+e2,2.0,contextptr);
	  gen e12=e2-e1;
	  double ex=evalf_double(re(e12,contextptr),1,contextptr)._DOUBLE_val,ey=evalf_double(im(e12,contextptr),1,contextptr)._DOUBLE_val;
	  if (!Mon_image.findij(centre,x_scale,y_scale,i0,j0,contextptr))
	    return;
	  gen diam=std::sqrt(ex*ex+ey*ey);
	  gen angle=std::atan2(ey,ex);
	  gen a1=v[1].evalf_double(1,contextptr),a2=v[2].evalf_double(1,contextptr);
	  bool full=v[1]==0 && v[2]==cst_two_pi;
	  if ( (diam.type==_DOUBLE_) && (a1.type==_DOUBLE_) && (a2.type==_DOUBLE_) ){
	    i1=diam._DOUBLE_val*x_scale/2.0;
	    j1=diam._DOUBLE_val*y_scale/2.0;
	    double a1d=a1._DOUBLE_val,a2d=a2._DOUBLE_val,angled=angle._DOUBLE_val;
	    bool changer_sens=a1d>a2d;
	    if (changer_sens){
	      double tmp=a1d;
	      a1d=a2d;
	      a2d=tmp;
	    }
	    double anglei=(angled+a1d),anglef=(angled+a2d),anglem=(anglei+anglef)/2;
	    if (fill_polygon)
	      fl_pie(deltax+round(i0-i1),deltay+round(j0-j1),round(2*i1),round(2*j1),full?0:anglei*180/M_PI+.5,full?360:anglef*180/M_PI+.5,couleur,false);
	    else {
	      fl_arc(deltax+round(i0-i1),deltay+round(j0-j1),round(2*i1),round(2*j1),full?0:anglei*180/M_PI+.5,full?360:anglef*180/M_PI+.5,couleur);
	      if (v.size()>=4){ // if cercle has the optionnal 5th arg
		if (v[3]==2)
		  petite_fleche(i0+i1*std::cos(anglem),j0-j1*std::sin(anglem),-i1*std::sin(anglem),-j1*std::cos(anglem),deltax,deltay,width,couleur);
		else {
		  if (changer_sens)
		    petite_fleche(i0+i1*std::cos(anglei),j0-j1*std::sin(anglei),-i1*std::sin(anglei),-j1*std::cos(anglei),deltax,deltay,width,couleur);
		  else
		    petite_fleche(i0+i1*std::cos(anglef),j0-j1*std::sin(anglef),i1*std::sin(anglef),j1*std::cos(anglef),deltax,deltay,width,couleur);
		}
	      }
	    }
	    // Label a few degrees from the start angle, 
	    // FIXME should use labelpos
	    double anglel=angled+a1d+0.3;
	    if (v.size()>=4 && v[3]==2)
	      anglel=angled+(0.45*a1d+0.55*a2d);
	    i0=i0+i1*std::cos(anglel); 
	    j0=j0-j1*std::sin(anglel);
	    if (!hidden_name)
	      draw_legende(f,round(i0),round(j0),labelpos,&Mon_image,clip_x,clip_y,clip_w,clip_h,0,0,couleur,contextptr);
	    return;
	  }
	} // end circle
#if 0
	if (point._SYMBptr->sommet==at_legende){
	  gen & f=point._SYMBptr->feuille;
	  if (f.type==_VECT && f._VECTptr->size()==3){
	    vecteur & fv=*f._VECTptr;
	    if (fv[0].type==_VECT && fv[0]._VECTptr->size()>=2 && fv[1].type==_STRNG && fv[2].type==_INT_){
	      vecteur & fvv=*fv[0]._VECTptr;
	      if (fvv[0].type==_INT_ && fvv[1].type==_INT_){
		int dx=0,dy=0;
		string legendes(*fv[1]._STRNGptr);
		find_dxdy(legendes,labelpos,fontsize,dx,dy);
		text_print(fontsize,legendes.c_str(),deltax+fvv[0].val+dx,deltay+fvv[1].val+dy,fv[2].val);
	      }
	    }
	  }
	}
#endif
      } // end point.type==_SYMB
      if (point.type!=_VECT || (point.type==_VECT && (point.subtype==_GROUP__VECT || point.subtype==_VECTOR__VECT) && point._VECTptr->size()==2 && is_zero(point._VECTptr->back()-point._VECTptr->front())) ){ // single point
	if (!Mon_image.findij((point.type==_VECT?point._VECTptr->front():point),x_scale,y_scale,i0,j0,contextptr))
	  return;
	if (i0>0 && i0<mxw && j0>0 && j0<myw)
	  fltk_point(deltax,deltay,round(i0),round(j0),epaisseur_point,type_point,couleur);
	if (!hidden_name)
	  draw_legende(f,round(i0),round(j0),labelpos,&Mon_image,clip_x,clip_y,clip_w,clip_h,0,0,couleur,contextptr);
	return;
      }
      // path
      const_iterateur jt=point._VECTptr->begin(),jtend=point._VECTptr->end();
      if (jt==jtend)
	return;
      bool logx=Mon_image.display_mode & 0x400,logy=Mon_image.display_mode & 0x800;
      if (jt->type==_VECT)
	return;
      if ( (type_point || epaisseur_point>2) && type_line==0 && width==0){
	for (;jt!=jtend;++jt){
	  if (!Mon_image.findij(*jt,x_scale,y_scale,i0,j0,contextptr))
	    return;
	  if (i0>0 && i0<mxw && j0>0 && j0<myw)
	    fltk_point(deltax,deltay,round(i0),round(j0),epaisseur_point,type_point,couleur);
	}
	if (!hidden_name)
	  draw_legende(f,round(i0),round(j0),labelpos,&Mon_image,clip_x,clip_y,clip_w,clip_h,0,0,couleur,contextptr);
	return;
      }
      // initial point
      if (!Mon_image.findij(*jt,x_scale,y_scale,i0,j0,contextptr))
	return;
      i0save=i0;
      j0save=j0;
      if (fill_polygon){
	if (jtend-jt==5 && *(jt+4)==*jt){
	  // check rectangle parallel to axes -> draw_rectangle (filled)
	  int cote1=horiz_or_vert(jt,contextptr);
	  if (cote1 && horiz_or_vert(jt+1,contextptr)==3-cote1 && horiz_or_vert(jt+2,contextptr)==cote1 && horiz_or_vert(jt+3,contextptr)==3-cote1){
	    if (!Mon_image.findij(*(jt+2),x_scale,y_scale,i0,j0,contextptr))
	      return;
	    int x,y,w,h;
	    if (i0<i0save){
	      x=i0;
	      w=i0save-i0;
	    }
	    else {
	      x=i0save;
	      w=i0-i0save;
	    }
	    if (j0<j0save){
	      y=j0;
	      h=j0save-j0;
	    }
	    else {
	      y=j0save;
	      h=j0-j0save;
	    }
	    draw_rectangle(deltax+x,deltay+y,w,h,couleur);
	    return;
	  }
	} // end rectangle check
	bool closed=*jt==*(jtend-1);
	vector< vector<int> > vi(jtend-jt+(closed?0:1),vector<int>(2));
	for (int pos=0;jt!=jtend;++pos,++jt){
	  if (!Mon_image.findij(*jt,x_scale,y_scale,i0,j0,contextptr))
	    return;
	  vi[pos][0]=i0+deltax;
	  vi[pos][1]=j0+deltay;
	}
	if (!closed)
	  vi.back()=vi.front();
	draw_filled_polygon(vi,0,LCD_WIDTH_PX,24,LCD_HEIGHT_PX,couleur);
	return;
      }
      ++jt;
      if (jt==jtend){
	if (i0>0 && i0<mxw && j0>0 && j0<myw)
	  check_fl_point(deltax+round(i0),deltay+round(j0),clip_x,clip_y,clip_w,clip_h,0,0,couleur);
	if (!hidden_name)
	  draw_legende(f,round(i0),round(j0),labelpos,&Mon_image,clip_x,clip_y,clip_w,clip_h,0,0,couleur,contextptr);
	return;
      }
      bool seghalfline=( point.subtype==_LINE__VECT || point.subtype==_HALFLINE__VECT ) && (point._VECTptr->size()==2);
      // rest of the path
      for (;;){
	if (!Mon_image.findij(*jt,x_scale,y_scale,i1,j1,contextptr))
	  return;
	if (!seghalfline){
	  checklog_fl_line(i0,j0,i1,j1,deltax,deltay,logx,logy,Mon_image.window_xmin,x_scale,Mon_image.window_ymax,y_scale,couleur);
	  if (point.subtype==_VECTOR__VECT){
	    double dx=i0-i1,dy=j0-j1;
	    petite_fleche(i1,j1,dx,dy,deltax,deltay,width,couleur);
	  }
	}
	++jt;
	if (jt==jtend){ // label of line at midpoint
	  if (point.subtype==_LINE__VECT){
	    i0=(6*i1-i0)/5-8;
	    j0=(6*j1-j0)/5-8;
	  }
	  else {
	    i0=(i0+i1)/2-8;
	    j0=(j0+j1)/2;
	  }
	  break;
	}
	i0=i1;
	j0=j1;
      }
      // check for a segment/halfline/line
      if ( seghalfline){
	double deltai=i1-i0save,adeltai=absdouble(deltai);
	double deltaj=j1-j0save,adeltaj=absdouble(deltaj);
	if (point.subtype==_LINE__VECT){
	  if (deltai==0)
	    checklog_fl_line(i1,0,i1,clip_h,deltax,deltay,logx,logy,Mon_image.window_xmin,x_scale,Mon_image.window_ymax,y_scale,couleur);
	  else {
	    if (deltaj==0)
	      checklog_fl_line(0,j1,clip_w,j1,deltax,deltay,Mon_image.display_mode & 0x400,Mon_image.display_mode & 0x800,Mon_image.window_xmin,x_scale,Mon_image.window_ymax,y_scale,couleur);
	    else {
	      // Find the intersections with the 4 rectangle segments
	      // Horizontal x=0 or w =i1+t*deltai: y=j1+t*deltaj
	      vector< complex<double> > pts;
	      double y0=j1-i1/deltai*deltaj,tol=clip_h*1e-6;
	      if (y0>=-tol && y0<=clip_h+tol)
		pts.push_back(complex<double>(0.0,y0));
	      double yw=j1+(clip_w-i1)/deltai*deltaj;
	      if (yw>=-tol && yw<=clip_h+tol)
		pts.push_back(complex<double>(clip_w,yw));
	      // Vertical y=0 or h=j1+t*deltaj, x=i1+t*deltai
	      double x0=i1-j1/deltaj*deltai;
	      tol=clip_w*1e-6;
	      if (x0>=-tol && x0<=clip_w+tol)
		pts.push_back(complex<double>(x0,0.0));
	      double xh=i1+(clip_h-j1)/deltaj*deltai;
	      if (xh>=-tol && xh<=clip_w+tol)
		pts.push_back(complex<double>(xh,clip_h));
	      if (pts.size()>=2)
		checklog_fl_line(pts[0].real(),pts[0].imag(),pts[1].real(),pts[1].imag(),deltax,deltay,Mon_image.display_mode & 0x400,Mon_image.display_mode & 0x800,Mon_image.window_xmin,x_scale,Mon_image.window_ymax,y_scale,couleur);
	    } // end else adeltai==0 , adeltaj==0
	  } // end else adeltai==0
	} // end LINE_VECT
	else {
	  double N=1;
	  if (adeltai){
	    N=clip_w/adeltai+1;
	    if (adeltaj)
	      N=max(N,clip_h/adeltaj+1);
	  }
	  else {
	    if (adeltaj)
	      N=clip_h/adeltaj+1;
	  }
	  N *= 2; // increase N since rounding might introduce too small clipping
	  while (fabs(N*deltai)>10000)
	    N /= 2;
	  while (fabs(N*deltaj)>10000)
	    N /= 2;
	  checklog_fl_line(i0save,j0save,i1+N*deltai,j1+N*deltaj,deltax,deltay,Mon_image.display_mode & 0x400,Mon_image.display_mode & 0x800,Mon_image.window_xmin,x_scale,Mon_image.window_ymax,y_scale,couleur);
	}
      } // end seghalfline
      if ( (point.subtype==_GROUP__VECT) && (point._VECTptr->size()==2))
	; // no legend for segment
      else {
	if (!hidden_name)
	  draw_legende(f,round(i0),round(j0),labelpos,&Mon_image,clip_x,clip_y,clip_w,clip_h,0,0,couleur,contextptr);
      }
    } // end pnt subcase
  }
#endif

  // return a vector of values with simple decimal representation
  // between xmin/xmax or including xmin/xmax (if bounds is true)
  vecteur ticks(double xmin,double xmax,bool bounds){
    if (xmax<xmin)
      swapdouble(xmin,xmax);
    double dx=xmax-xmin;
    vecteur res;
    if (dx==0)
      return res;
    double d=std::pow(10.0,std::floor(std::log10(dx)));
    if (dx<2*d)
      d=d/5;
    else {
      if (dx<5*d)
	d=d/2;
    }
    double x1=std::floor(xmin/d)*d;
    double x2=(bounds?std::ceil(xmax/d):std::floor(xmax/d))*d;
    for (double x=x1+(bounds?0:d);x<=x2;x+=d){
      if (absdouble(x-int(x+.5))<1e-6*d)
	res.push_back(int(x+.5));
      else
	res.push_back(x);
    }
    return res;
  }

  void Graph2d::draw(){
    int save_clip_ymin=clip_ymin;
    clip_ymin=STATUS_AREA_PX;
    int horizontal_pixels=LCD_WIDTH_PX,vertical_pixels=LCD_HEIGHT_PX-STATUS_AREA_PX,deltax=0,deltay=STATUS_AREA_PX,clip_x=0,clip_y=0,clip_w=horizontal_pixels,clip_h=vertical_pixels;
    drawRectangle(0, STATUS_AREA_PX, horizontal_pixels, vertical_pixels,COLOR_WHITE);
    // Draw axis
    double I0,J0;
    findij(zero,x_scale,y_scale,I0,J0,contextptr); // origin
    int i_0=round(I0),j_0=round(J0);
    if (show_axes &&  (window_ymax>=0) && (window_ymin<=0)){ // X-axis
      vecteur aff; int affs;
      char ch[256];
      check_fl_line(deltax,deltay+j_0,deltax+horizontal_pixels,deltay+j_0,clip_x,clip_y,clip_w,clip_h,0,0,_GREEN); 
      check_fl_line(deltax+i_0,deltay+j_0,deltax+i_0+int(x_scale),deltay+j_0,clip_x,clip_y,clip_w,clip_h,0,0,_CYAN);
      aff=ticks(window_xmin,window_xmax,true);
      affs=aff.size();
      for (int i=0;i<affs;++i){
	double d=evalf_double(aff[i],1,contextptr)._DOUBLE_val;
	if (fabs(d)<1e-6) strcpy(ch,"0"); else sprint_double(ch,d);
	int delta=int(horizontal_pixels*(d-window_xmin)/(window_xmax-window_xmin));
	int taille=strlen(ch)*9;
	fl_line(delta,deltay+j_0,delta,deltay+j_0-4,_GREEN);
      }
      check_fl_draw(labelsize,"x",deltax+horizontal_pixels-40,deltay+j_0-4,clip_x,clip_y,clip_w,clip_h,0,0,_GREEN);
    }
    if ( show_axes && (window_xmax>=0) && (window_xmin<=0) ) {// Y-axis
      vecteur aff; int affs;
      char ch[256];
      check_fl_line(deltax+i_0,deltay,deltax+i_0,deltay+vertical_pixels,clip_x,clip_y,clip_w,clip_h,0,0,_RED);
      check_fl_line(deltax+i_0,deltay+j_0,deltax+i_0,deltay+j_0-int(y_scale),clip_x,clip_y,clip_w,clip_h,0,0,_CYAN);
      aff=ticks(window_ymin,window_ymax,true);
      affs=aff.size();
      int taille=5;
      for (int j=0;j<affs;++j){
	double d=evalf_double(aff[j],1,contextptr)._DOUBLE_val;
	if (fabs(d)<1e-6) strcpy(ch,"0"); else sprint_double(ch,d);
	int delta=int(vertical_pixels*(window_ymax-d)/(window_ymax-window_ymin));
	if (delta>=taille && delta<=vertical_pixels-taille){
	  fl_line(deltax+i_0,STATUS_AREA_PX+delta,deltax+i_0+4,STATUS_AREA_PX+delta,_RED);
	}
      }
      check_fl_draw(labelsize,"y",deltax+i_0+2,deltay+labelsize,clip_x,clip_y,clip_w,clip_h,0,0,_RED);
    }
#if 0 // if ticks are enabled, don't forget to set freeze to false
    // Ticks
    if (show_axes && (horizontal_pixels)/(x_scale*x_tick) < 40 && vertical_pixels/(y_tick*y_scale) <40  ){
      if (x_tick>0 && y_tick>0 ){
	double nticks=(horizontal_pixels-I0)/(x_scale*x_tick);
	double mticks=(vertical_pixels-J0)/(y_tick*y_scale);
	int count=0;
	for (int ii=int(-I0/(x_tick*x_scale));ii<=nticks;++ii){
	  int iii=int(I0+ii*x_scale*x_tick+.5);
	  for (int jj=int(-J0/(y_tick*y_scale));jj<=mticks && count<1600;++jj,++count){
	    int jjj=int(J0+jj*y_scale*y_tick+.5);
	    check_fl_point(deltax+iii,deltay+jjj,clip_x,clip_y,clip_w,clip_h,0,0,COLOR_BLACK);
	  }
	}
      }
    }
#endif
    if (show_axes){ 
      int taille,affs,delta;
      vecteur aff;
      char ch[256];
      // X
      aff=ticks(window_xmin,window_xmax,true);
      affs=aff.size();
      for (int i=0;i<affs;++i){
	double d=evalf_double(aff[i],1,contextptr)._DOUBLE_val;
	sprint_double(ch,d);
	delta=int(horizontal_pixels*(d-window_xmin)/(window_xmax-window_xmin));
	taille=strlen(ch)*9;
	fl_line(delta,vertical_pixels+STATUS_AREA_PX-6,delta,vertical_pixels+STATUS_AREA_PX-1,_GREEN);
	if (delta>=taille/2 && delta<=horizontal_pixels){
	  text_print(10,ch,delta-taille/2,vertical_pixels+STATUS_AREA_PX-7,_GREEN);
	}
      }
      // Y
      aff=ticks(window_ymin,window_ymax,true);
      affs=aff.size();
      taille=5;
      for (int j=0;j<affs;++j){
	double d=evalf_double(aff[j],1,contextptr)._DOUBLE_val;
	sprint_double(ch,d);
	delta=int(vertical_pixels*(window_ymax-d)/(window_ymax-window_ymin));
	if (delta>=taille && delta<=vertical_pixels-taille){
	  fl_line(horizontal_pixels-5,STATUS_AREA_PX+delta,horizontal_pixels-1,STATUS_AREA_PX+delta,_RED);
	  text_print(10,ch,horizontal_pixels-strlen(ch)*9,STATUS_AREA_PX+delta+taille,_RED);
	}
      }
    }
    
    // draw
    fltk_draw(*this,g,x_scale,y_scale,clip_x,clip_y,clip_w,clip_h,contextptr);
    clip_ymin=save_clip_ymin;
  }
  
  void Graph2d::left(double d){ 
    window_xmin -= d;
    window_xmax -= d;
  }

  void Graph2d::right(double d){ 
    window_xmin += d;
    window_xmax += d;
  }

  void Graph2d::up(double d){ 
    window_ymin += d;
    window_ymax += d;
  }

  void Graph2d::down(double d){ 
    window_ymin -= d;
    window_ymax -= d;
  }

  void Turtle::draw(){
    const int deltax=0,deltay=0;
    int horizontal_pixels=LCD_WIDTH_PX-2*giac::COORD_SIZE;
    // Check for fast redraw
    // Then redraw the background
    drawRectangle(deltax, deltay, LCD_WIDTH_PX, LCD_HEIGHT_PX-24,COLOR_WHITE);
    if (turtleptr &&
#ifdef TURTLETAB
	turtle_stack_size
#else
	!turtleptr->empty()
#endif
	){
      if (turtlezoom>8)
	turtlezoom=8;
      if (turtlezoom<0.125)
	turtlezoom=0.125;
      // check that position is not out of screen
#ifdef TURTLETAB
      logo_turtle t=turtleptr[turtle_stack_size-1];
#else
      logo_turtle t=turtleptr->back();
#endif
      double x=turtlezoom*(t.x-turtlex);
      if (x<0)
	turtlex += int(x/turtlezoom);
      if (x>=LCD_WIDTH_PX-10)
	turtlex += int((x-LCD_WIDTH_PX+10)/turtlezoom);
      double y=turtlezoom*(t.y-turtley);
      if (y<0)
	turtley += int(y/turtlezoom);
      if (y>LCD_HEIGHT_PX-10)
	turtley += int((y-LCD_HEIGHT_PX+10)/turtlezoom);
    }
#if 0
    if (maillage & 0x3){
      fl_color(FL_BLACK);
      double xdecal=std::floor(turtlex/10.0)*10;
      double ydecal=std::floor(turtley/10.0)*10;
      if ( (maillage & 0x3)==1){
	for (double i=xdecal;i<LCD_WIDTH_PX+xdecal;i+=10){
	  for (double j=ydecal;j<LCD_HEIGHT_PX+ydecal;j+=10){
	    fl_point(deltax+int((i-turtlex)*turtlezoom+.5),deltay+LCD_HEIGHT_PX-int((j-turtley)*turtlezoom+.5));
	  }
	}
      }
      else {
	double dj=std::sqrt(3.0)*10,i0=xdecal;
	for (double j=ydecal;j<LCD_HEIGHT_PX+ydecal;j+=dj){
	  int J=deltay+int(LCD_HEIGHT_PX-(j-turtley)*turtlezoom);
	  for (double i=i0;i<LCD_WIDTH_PX+xdecal;i+=10){
	    fl_point(deltax+int((i-turtlex)*turtlezoom+.5),J);
	  }
	  i0 += dj;
	  while (i0>=10)
	    i0 -= 10;
	}
      }
    }
#endif
    // Show turtle position/cap
    if (turtleptr &&
#ifdef TURTLETAB
	turtle_stack_size &&
#else
	!turtleptr->empty() &&
#endif
	!(maillage & 0x4)){
#ifdef TURTLETAB
      logo_turtle turtle=turtleptr[turtle_stack_size-1];
#else
      logo_turtle turtle=turtleptr->back();
#endif
      drawRectangle(deltax+horizontal_pixels,deltay,LCD_WIDTH_PX-horizontal_pixels,2*COORD_SIZE,_YELLOW);
      // drawRectangle(deltax, deltay, LCD_WIDTH_PX, LCD_HEIGHT_PX,COLOR_BLACK);
      char buf[32];
      sprintf(buf,"x %i   ",int(turtle.x+.5));
      text_print(18,buf,deltax+horizontal_pixels,deltay+(2*COORD_SIZE)/3-2,COLOR_BLACK,_YELLOW);
      sprintf(buf,"y %i   ",int(turtle.y+.5));
      text_print(18,buf,deltax+horizontal_pixels,deltay+(4*COORD_SIZE)/3-3,COLOR_BLACK,_YELLOW);
      sprintf(buf,"t %i   ",int(turtle.theta+.5));
      text_print(18,buf,deltax+horizontal_pixels,deltay+2*COORD_SIZE-4,COLOR_BLACK,_YELLOW);
    }
    // draw turtle Logo
    if (turtleptr){
#ifdef TURTLETAB
      int l=turtle_stack_size;
#else
      int l=turtleptr->size();
#endif
      if (l>0){
#ifdef TURTLETAB
	logo_turtle prec =turtleptr[0];
#else
	logo_turtle prec =(*turtleptr)[0];
#endif
	for (int k=1;k<l;++k){
#ifdef TURTLETAB
	  logo_turtle current =(turtleptr)[k];
#else
	  logo_turtle current =(*turtleptr)[k];
#endif
#if 1
	  if (current.s>=0){ // Write a string
	    //cout << current.radius << " " << current.s << endl;
	    if (current.s<ecristab().size())
	      text_print(current.radius,ecristab()[current.s].c_str(),int(deltax+turtlezoom*(current.x-turtlex)),int(deltay+LCD_HEIGHT_PX-turtlezoom*(current.y-turtley)),current.color);
	  }
	  else
#endif
	    {
	    if (current.radius>0){
	      int r=current.radius & 0x1ff; // bit 0-8
	      double theta1,theta2;
	      if (current.direct){
		theta1=prec.theta+double((current.radius >> 9) & 0x1ff); // bit 9-17
		theta2=prec.theta+double((current.radius >> 18) & 0x1ff); // bit 18-26
	      }
	      else {
		theta1=prec.theta-double((current.radius >> 9) & 0x1ff); // bit 9-17
		theta2=prec.theta-double((current.radius >> 18) & 0x1ff); // bit 18-26
	      }
	      bool rempli=(current.radius >> 27) & 0x1;
	      bool seg=(current.radius >> 28) & 0x1;
	      double angle;
	      int x,y,R;
	      R=int(2*turtlezoom*r+.5);
	      angle = M_PI/180*(theta2-90);
	      if (current.direct){
		x=int(turtlezoom*(current.x-turtlex-r*std::cos(angle) - r)+.5);
		y=int(turtlezoom*(current.y-turtley-r*std::sin(angle) + r)+.5);
	      }
	      else {
		x=int(turtlezoom*(current.x-turtlex+r*std::cos(angle) -r)+.5);
		y=int(turtlezoom*(current.y-turtley+r*std::sin(angle) +r)+.5);
	      }
	      if (current.direct){
		if (rempli)
		  fl_pie(deltax+x,deltay+LCD_HEIGHT_PX-y,R,R,theta1-90,theta2-90,current.color,seg);
		else
		  fl_arc(deltax+x,deltay+LCD_HEIGHT_PX-y,R,R,theta1-90,theta2-90,current.color);
	      }
	      else {
		if (rempli)
		  fl_pie(deltax+x,deltay+LCD_HEIGHT_PX-y,R,R,90+theta2,90+theta1,current.color,seg);
		else
		  fl_arc(deltax+x,deltay+LCD_HEIGHT_PX-y,R,R,90+theta2,90+theta1,current.color);
	      }
	    } // end radius>0
	    else {
	      if (prec.mark){
		fl_line(deltax+int(turtlezoom*(prec.x-turtlex)+.5),deltay+int(LCD_HEIGHT_PX+turtlezoom*(turtley-prec.y)+.5),deltax+int(turtlezoom*(current.x-turtlex)+.5),deltay+int(LCD_HEIGHT_PX+turtlezoom*(turtley-current.y)+.5),prec.color);
	      }
	    }
	    if (current.radius<-1 && k+current.radius>=0){
	      // poly-line from (*turtleptr)[k+current.radius] to (*turtleptr)[k]
	      vector< vector<int> > vi(1-current.radius,vector<int>(2));
	      for (int i=0;i>=current.radius;--i){
#ifdef TURTLETAB
		logo_turtle & t=(turtleptr)[k+i];
#else
		logo_turtle & t=(*turtleptr)[k+i];
#endif
		vi[-i][0]=deltax+turtlezoom*(t.x-turtlex);
		vi[-i][1]=deltay+LCD_HEIGHT_PX+turtlezoom*(turtley-t.y);
		//*logptr(contextptr) << i << " " << vi[-i][0] << " " << vi[-i][1] << endl;
	      }
	      //vi.back()=vi.front();
	      draw_filled_polygon(vi,0,LCD_WIDTH_PX,24,LCD_HEIGHT_PX,current.color);
	    }
	  } // end else (non-string turtle record)
	  prec=current;
	} // end for (all turtle records)
#ifdef TURTLETAB
	logo_turtle & t = (turtleptr)[l-1];
#else
	logo_turtle & t = (*turtleptr)[l-1];
#endif
	int x=int(turtlezoom*(t.x-turtlex)+.5);
	int y=int(turtlezoom*(t.y-turtley)+.5);
	double cost=std::cos(t.theta*deg2rad_d);
	double sint=std::sin(t.theta*deg2rad_d);
	int Dx=int(turtlezoom*t.turtle_length*cost/2+.5);
	int Dy=int(turtlezoom*t.turtle_length*sint/2+.5);
	if (t.visible){
	  fl_line(deltax+x+Dy,deltay+LCD_HEIGHT_PX-(y-Dx),deltax+x-Dy,deltay+LCD_HEIGHT_PX-(y+Dx),t.color);
	  int c=t.color;
	  if (!t.mark)
	    c=t.color ^ 0x7777;
	  fl_line(deltax+x+Dy,deltay+LCD_HEIGHT_PX-(y-Dx),deltax+x+3*Dx,deltay+LCD_HEIGHT_PX-(y+3*Dy),c);
	  fl_line(deltax+x-Dy,deltay+LCD_HEIGHT_PX-(y+Dx),deltax+x+3*Dx,deltay+LCD_HEIGHT_PX-(y+3*Dy),c);
	}
      }
      return;
    } // End logo mode
  }  
  

  void displaygraph(const giac::gen & ge,GIAC_CONTEXT){
    // graph display
    //if (aborttimer > 0) { Timer_Stop(aborttimer); Timer_Deinstall(aborttimer);}
    xcas::Graph2d gr(ge,contextptr);
    gr.show_axes=true;
    // initial setting for x and y
    if (ge.type==_VECT){
      const_iterateur it=ge._VECTptr->begin(),itend=ge._VECTptr->end();
      for (;it!=itend;++it){
	if (it->is_symb_of_sommet(at_equal)){
	  const gen & f=it->_SYMBptr->feuille;
	  gen & optname = f._VECTptr->front();
	  gen & optvalue= f._VECTptr->back();
	  if (optname.val==_AXES && optvalue.type==_INT_)
	    gr.show_axes=optvalue.val;
	  if (optname.type==_INT_ && optname.subtype == _INT_PLOT && optname.val>=_GL_X && optname.val<=_GL_Z && optvalue.is_symb_of_sommet(at_interval)){
	    //*logptr(contextptr) << optname << " " << optvalue << endl;
	    gen optvf=evalf_double(optvalue._SYMBptr->feuille,1,contextptr);
	    if (optvf.type==_VECT && optvf._VECTptr->size()==2){
	      gen a=optvf._VECTptr->front();
	      gen b=optvf._VECTptr->back();
	      if (a.type==_DOUBLE_ && b.type==_DOUBLE_){
		switch (optname.val){
		case _GL_X:
		  gr.window_xmin=a._DOUBLE_val;
		  gr.window_xmax=b._DOUBLE_val;
		  gr.update();
		  break;
		case _GL_Y:
		  gr.window_ymin=a._DOUBLE_val;
		  gr.window_ymax=b._DOUBLE_val;
		  gr.update();
		  break;
		}
	      }
	    }
	  }
	}
      }
    }
    // UI
    DefineStatusMessage((char*)"+-: zoom, pad: move, EXIT: quit", 1, 0, 0);
    // EnableStatusArea(2);
    for (;;){
      gr.draw();
      DisplayStatusArea();
      // int x=0,y=LCD_HEIGHT_PX-STATUS_AREA_PX-17;
      // PrintMini(&x,&y,(unsigned char *)"menu",0x04,0xffffffff,0,0,COLOR_BLACK,COLOR_WHITE,1,0);
      int key=-1;
      GetKey(&key);
#if 1
      if (key==KEY_CTRL_CATALOG){
	char menu_xmin[32],menu_xmax[32],menu_ymin[32],menu_ymax[32];
	string s;
	s="xmin "+print_DOUBLE_(gr.window_xmin,6);
	strcpy(menu_xmin,s.c_str());
	s="xmax "+print_DOUBLE_(gr.window_xmax,6);
	strcpy(menu_xmax,s.c_str());
	s="ymin "+print_DOUBLE_(gr.window_ymin,6);
	strcpy(menu_ymin,s.c_str());
	s="ymax "+print_DOUBLE_(gr.window_ymax,6);
	strcpy(menu_ymax,s.c_str());
	Menu smallmenu;
	smallmenu.numitems=12;
	MenuItem smallmenuitems[smallmenu.numitems];
	smallmenu.items=smallmenuitems;
	smallmenu.height=12;
	//smallmenu.title = "KhiCAS";
	smallmenuitems[0].text = (char *) menu_xmin;
	smallmenuitems[1].text = (char *) menu_xmax;
	smallmenuitems[2].text = (char *) menu_ymin;
	smallmenuitems[3].text = (char *) menu_ymax;
	smallmenuitems[4].text = (char*) "Orthonormalize /";
	smallmenuitems[5].text = (char*) "Autoscale *";
	smallmenuitems[6].text = (char *) ("Zoom in +");
	smallmenuitems[7].text = (char *) ("Zoom out -");
	smallmenuitems[8].text = (char *) ("Y-Zoom out (-)");
	smallmenuitems[9].text = (char*) (lang?"Voir axes":"Show axes");
	smallmenuitems[10].text = (char*) (lang?"Cacher axes":"Hide axes");
	smallmenuitems[11].text = (char*)(lang?"Quitter":"Quit");
	int sres = doMenu(&smallmenu);
	if(sres == MENU_RETURN_SELECTION) {
	  const char * ptr=0;
	  string s1; double d;
	  if (smallmenu.selection==1){
	    if (inputdouble(menu_xmin,d,contextptr)){
	      gr.window_xmin=d;
	      gr.update();
	    }
	  }
	  if (smallmenu.selection==2){
	    if (inputdouble(menu_xmax,d,contextptr)){
	      gr.window_xmax=d;
	      gr.update();
	    }
	  }
	  if (smallmenu.selection==3){
	    if (inputdouble(menu_ymin,d,contextptr)){
	      gr.window_ymin=d;
	      gr.update();
	    }
	  }
	  if (smallmenu.selection==4){
	    if (inputdouble(menu_ymax,d,contextptr)){
	      gr.window_ymax=d;
	      gr.update();
	    }
	  }
	  if (smallmenu.selection==5)
	    gr.orthonormalize();
	  if (smallmenu.selection==6)
	    gr.autoscale();	
	  if (smallmenu.selection==7)
	    gr.zoom(0.7);	
	  if (smallmenu.selection==8)
	    gr.zoom(1/0.7);	
	  if (smallmenu.selection==9)
	    gr.zoomy(1/0.7);
	  if (smallmenu.selection==10)
	    gr.show_axes=true;	
	  if (smallmenu.selection==11)
	    gr.show_axes=false;	
	  if (smallmenu.selection==12)
	    break;
	}
      }
#endif
      if (key==KEY_CTRL_EXIT || key==KEY_CTRL_OK){
	numworks_giac_hide_graph();
	break;
      }
      if (key==KEY_CTRL_UP){ gr.up((gr.window_ymax-gr.window_ymin)/5); }
      if (key==KEY_CTRL_PAGEUP) { gr.up((gr.window_ymax-gr.window_ymin)/2); }
      if (key==KEY_CTRL_DOWN) { gr.down((gr.window_ymax-gr.window_ymin)/5); }
      if (key==KEY_CTRL_PAGEDOWN) { gr.down((gr.window_ymax-gr.window_ymin)/2);}
      if (key==KEY_CTRL_LEFT) { gr.left((gr.window_xmax-gr.window_xmin)/5); }
      if (key==KEY_SHIFT_LEFT) { gr.left((gr.window_xmax-gr.window_xmin)/2); }
      if (key==KEY_CTRL_RIGHT) { gr.right((gr.window_xmax-gr.window_xmin)/5); }
      if (key==KEY_SHIFT_RIGHT) { gr.right((gr.window_xmax-gr.window_xmin)/5); }
      if (key==KEY_CHAR_PLUS) {
	gr.zoom(0.7);
      }
      if (key==KEY_CHAR_MINUS){
	gr.zoom(1/0.7);
      }
      if (key==KEY_CHAR_PMINUS){
	gr.zoomy(1/0.7);
      }
      if (key==KEY_CHAR_MULT){
	gr.autoscale();
      }
      if (key==KEY_CHAR_DIV) {
	gr.orthonormalize();
      }
      if (key==KEY_CTRL_VARS) {
	gr.show_axes=!gr.show_axes;
      }
    }
    // aborttimer = Timer_Install(0, check_execution_abort, 100); if (aborttimer > 0) { Timer_Start(aborttimer); }
    return ;
  }

  void displaylogo(){
#ifdef TURTLETAB
    xcas::Turtle t={tablogo,0,0,1,1};
#else
    xcas::Turtle t={&turtle_stack(),0,0,1,1};
#endif
    DefineStatusMessage((char*)"+-: zoom, pad: move, EXIT: quit", 1, 0, 0);
    DisplayStatusArea();
    while (1){
      int save_ymin=clip_ymin;
      clip_ymin=24;
      t.draw();
      clip_ymin=save_ymin;
      int key;
      GetKey(&key);
      if (key==KEY_CTRL_EXIT || key==KEY_CTRL_OK || key==KEY_PRGM_ACON || key==KEY_CTRL_MENU || key==KEY_CTRL_EXE || key==KEY_CTRL_VARS)
	break;
      if (key==KEY_CTRL_UP){ t.turtley += 10; }
      if (key==KEY_CTRL_PAGEUP) { t.turtley += 100; }
      if (key==KEY_CTRL_DOWN) { t.turtley -= 10; }
      if (key==KEY_CTRL_PAGEDOWN) { t.turtley -= 100;}
      if (key==KEY_CTRL_LEFT) { t.turtlex -= 10; }
      if (key==KEY_SHIFT_LEFT) { t.turtlex -= 100; }
      if (key==KEY_CTRL_RIGHT) { t.turtlex += 10; }
      if (key==KEY_SHIFT_RIGHT) { t.turtlex += 100;}
      if (key==KEY_CHAR_PLUS) { t.turtlezoom *= 2;}
      if (key==KEY_CHAR_MINUS){ t.turtlezoom /= 2;  }
    }
    numworks_giac_hide_graph();
  }

  bool ispnt(const gen & g){
    if (g.is_symb_of_sommet(giac::at_pnt))
      return true;
    if (g.type!=_VECT || g._VECTptr->empty())
      return false;
    return ispnt(g._VECTptr->back());
  }

  giac::gen eqw(const giac::gen & ge,bool editable,GIAC_CONTEXT){
    bool edited=false;
    const int margin=16;
#ifdef CURSOR
    Cursor_SetFlashOff();
#endif
    giac::gen geq(_copy(ge,contextptr));
    // if (ge.type!=giac::_DOUBLE_ && giac::has_evalf(ge,geq,1,contextptr)) geq=giac::symb_equal(ge,geq);
    int line=-1,col=-1,nlines=0,ncols=0,listormat=0;
    xcas::Equation eq(0,0,geq,contextptr);
    giac::eqwdata eqdata=xcas::Equation_total_size(eq.data);
    if (eqdata.dx>1.5*LCD_WIDTH_PX || eqdata.dy>1.5*LCD_HEIGHT_PX){
      if (eqdata.dx>2.25*LCD_WIDTH_PX || eqdata.dy>2.25*LCD_HEIGHT_PX)
	eq.attr=giac::attributs(14,COLOR_WHITE,COLOR_BLACK);
      else
	eq.attr=giac::attributs(16,COLOR_WHITE,COLOR_BLACK);
      eq.data=0; // clear memory
      eq.data=xcas::Equation_compute_size(geq,eq.attr,LCD_WIDTH_PX,contextptr);
      eqdata=xcas::Equation_total_size(eq.data);
    }
    int dx=(eqdata.dx-LCD_WIDTH_PX)/2,dy=LCD_HEIGHT_PX-2*margin+eqdata.y;
    if (geq.type==_VECT){
      nlines=geq._VECTptr->size();
      if (eqdata.dx>=LCD_WIDTH_PX)
	dx=-20; // line=nlines/2;
      //else
      if (geq.subtype!=_SEQ__VECT){
	line=0;
	listormat=1;
	if (ckmatrix(geq)){
	  ncols=geq._VECTptr->front()._VECTptr->size();
	  if (eqdata.dy>=LCD_HEIGHT_PX-margin)
	    dy=eqdata.y+eqdata.dy+32;// col=ncols/2;
	  // else
	  col=0;
	  listormat=2;
	}
      }
    }
    if (!listormat){
      xcas::Equation_select(eq.data,true);
      xcas::eqw_select_down(eq.data);
    }
    //cout << eq.data << endl;
    int firstrun=2;
    for (;;){
#if 1
      if (firstrun==2){
	DefineStatusMessage((char*)(lang?"EXE: quitte, resultat dans last":"EXE: quit, result stored in last"), 1, 0, 0);
	DisplayStatusArea();
	firstrun=1;
      }
      else
	set_xcas_status();
#else
      DefineStatusMessage((char*)"+-: zoom, pad: move, EXIT: quit", 1, 0, 0);
      EnableStatusArea(2);
      DisplayStatusArea();
#endif
      gen value;
      if (listormat) // select line l, col c
	xcas::eqw_select(eq.data,line,col,true,value);
      if (eqdata.dx>LCD_WIDTH_PX){
	if (dx<-20)
	  dx=-20;
	if (dx>eqdata.dx-LCD_WIDTH_PX+20)
	  dx=eqdata.dx-LCD_WIDTH_PX+20;
      }
#define EQW_TAILLE 18
      if (eqdata.dy>LCD_HEIGHT_PX-2*EQW_TAILLE){
	if (dy-eqdata.y<LCD_HEIGHT_PX-2*EQW_TAILLE)
	  dy=eqdata.y+LCD_HEIGHT_PX-2*EQW_TAILLE;
	if (dy-eqdata.y>eqdata.dy+32)
	  dy=eqdata.y+eqdata.dy+32;
      }
      drawRectangle(0, STATUS_AREA_PX, LCD_WIDTH_PX, LCD_HEIGHT_PX-STATUS_AREA_PX,COLOR_WHITE);
      // Bdisp_AllClr_VRAM();
      int save_clip_ymin=clip_ymin;
      clip_ymin=STATUS_AREA_PX;
      xcas::display(eq,dx,dy,contextptr);
#if 0
      string menu(" ");
      menu += menu_f1;
      while (menu.size()<6)
	menu += " ";
      menu += " | ";
      menu += string(menu_f2);
      while (menu.size()<13)
	menu += " ";
      menu += " | edit+-| cmds | A<>a | eval";
      PrintMini(0,58,menu.c_str(),4);
#endif
      //draw_menu(2);
      clip_ymin=save_clip_ymin;
      int keyflag = GetSetupSetting( (unsigned int)0x14);
      if (firstrun){ // workaround for e.g. 1+x/2 partly not displayed
	firstrun=0;
	continue;
      }
      int key;
      //cout << eq.data << endl;
      GetKey(&key);
      bool alph=alphawasactive;
      if (key==KEY_CTRL_EXE)
	key=KEY_CTRL_F6;
      if (key==KEY_CTRL_OK){
	numworks_giac_hide_graph();
	if (edited && xcas::do_select(eq.data,true,value) && value.type==_EQW){
	  //cout << "ok " << value._EQWptr->g << endl;
	  //DefineStatusMessage((char*)lang?"resultat stocke dans last":"result stored in last", 1, 0, 0);
	  //DisplayStatusArea();
	  giac::sto(value._EQWptr->g,giac::gen("last",contextptr),contextptr);
	  return value._EQWptr->g;
	}
	//cout << "no " << eq.data << endl; if (value.type==_EQW) cout << value._EQWptr->g << endl ;
	return geq;
      }
      if (key==KEY_CTRL_EXIT || key==KEY_CTRL_AC ){
	if (!edited){
	  numworks_giac_hide_graph();
	  return geq;
	}
	if (confirm(lang?"Vraiment abandonner?":"Really leave",lang?"Back: editeur,  OK: confirmer":"Back: editor,  OK: confirm")==KEY_CTRL_F1){
	  numworks_giac_hide_graph();
	  return geq;
	}
      }
      if (key==KEY_CTRL_UNDO){
	giac::swapgen(eq.undodata,eq.data);
	continue;
      }
      if (key==KEY_CTRL_F5){
	handle_f5();
	continue;
      }
      int redo=0;
      bool ins=key==KEY_CHAR_STORE  || key==KEY_CHAR_RPAR || key==KEY_CHAR_LPAR || key==KEY_CHAR_COMMA || key==KEY_CTRL_PASTE;
      int xleft,ytop,xright,ybottom,gselpos; gen * gsel=0,*gselparent=0;
      if (key==KEY_CTRL_CLIP){
	xcas::Equation_adjust_xy(eq.data,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos,0);
	if (gsel==0)
	  gsel==&eq.data;
	// cout << "var " << g << " " << eq.data << endl;
	if (xcas::do_select(*gsel,true,value) && value.type==_EQW){
	  //cout << g << ":=" << value._EQWptr->g << endl;
	  copy_clipboard(value._EQWptr->g.print(contextptr),true);
	  continue;
	}
      }
      if (key==KEY_CHAR_STORE){
	int keyflag = GetSetupSetting( (unsigned int)0x14);
	if (keyflag==0)
	  handle_f5();
	std::string varname;
	if (inputline((lang?"Stocker la selection dans":"Save selection in",lang?"Nom de variable: ":"Variable name: "),0,varname,false,65,contextptr) && !varname.empty() && isalpha(varname[0])){
	  giac::gen g(varname,contextptr);
	  giac::gen ge(eval(g,1,contextptr));
	  if (g.type!=_IDNT){
	    invalid_varname();
	    continue;
	  }
	  if (ge==g || confirm_overwrite()){
	    vector<int> goto_sel;
	    xcas::Equation_adjust_xy(eq.data,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos,&goto_sel);
	    if (gsel==0)
	      gsel==&eq.data;
	    // cout << "var " << g << " " << eq.data << endl;
	    if (xcas::do_select(*gsel,true,value) && value.type==_EQW){
	      //cout << g << ":=" << value._EQWptr->g << endl;
	      giac::sto(value._EQWptr->g,g,contextptr);
	    }
	  }
	}
	continue;
      }
      if (key==KEY_CTRL_DEL){
	vector<int> goto_sel;
	if (xcas::Equation_adjust_xy(eq.data,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos,&goto_sel) && gsel && xcas::do_select(*gsel,true,value) && value.type==_EQW){
	  value=value._EQWptr->g;
	  if (value.type==_SYMB){
	    gen tmp=value._SYMBptr->feuille;
	    if (tmp.type!=_VECT || tmp.subtype!=_SEQ__VECT){
	      xcas::replace_selection(eq,tmp,gsel,&goto_sel,contextptr);
	      continue;
	    }
	  }
	  if (!goto_sel.empty() && gselparent && gselparent->type==_VECT && !gselparent->_VECTptr->empty()){
	    vecteur & v=*gselparent->_VECTptr;
	    if (v.back().type==_EQW){
	      gen opg=v.back()._EQWptr->g;
	      if (opg.type==_FUNC){
		int i=0;
		for (;i<v.size()-1;++i){
		  if (&v[i]==gsel)
		    break;
		}
		if (i<v.size()-1){
		  if (v.size()==5 && (opg==at_integrate || opg==at_sum) && i>=2)
		    v.erase(v.begin()+2,v.begin()+4);
		  else
		    v.erase(v.begin()+i);
		  xcas::do_select(*gselparent,true,value);
		  if (value.type==_EQW){
		    value=value._EQWptr->g;
		    // cout << goto_sel << " " << value << endl; continue;
		    if (v.size()==2 && (opg==at_plus || opg==at_prod))
		      value=eval(value,1,contextptr);
		    goto_sel.erase(goto_sel.begin());
		    xcas::replace_selection(eq,value,gselparent,&goto_sel,contextptr);
		    continue;
		  }
		}
	      }
	    }
	  }
	}
      }
      if (key=='\\'){
	xcas::do_select(eq.data,true,value);
	if (value.type==_EQW)
	  geq=value._EQWptr->g;
	if (eq.attr.fontsize<=14)
	  eq.attr.fontsize=18;
	else
	  eq.attr.fontsize=14;
	  redo=1;
      }
      if (key==KEY_CHAR_IMGNRY)
	key='i';
      const char keybuf[2]={(key==KEY_CHAR_PMINUS?'-':char(key)),0};
      const char * adds=(key==KEY_CHAR_PMINUS ||
			 (key==char(key) && (isalphanum(key)|| key=='.' ))
			 )?keybuf:keytostring(key,keyflag,contextptr);
      if (adds && !strcmp(adds,"VARS()"))
	continue;
      translate_fkey(key);
      if ( key==KEY_CTRL_F1 || key==KEY_CTRL_F2 ||
	   (key>=KEY_CTRL_F7 && key<=KEY_CTRL_F14)){
	adds=console_menu(key,1);//alph?"simplify":(keyflag==1?"factor":"partfrac");
	// workaround for infinitiy
	if (strlen(adds)>=2 && adds[0]=='o' && adds[1]=='o')
	  key=KEY_CTRL_F5;      
      }
      if (key==KEY_CTRL_F5)
	adds="oo";
      if (key==KEY_CTRL_F6){
	adds=alph?"regroup":(keyflag==1?"evalf":"eval");
      }
      if (key==KEY_CHAR_MINUS)
	adds="-";
      if (key==KEY_CHAR_EQUAL)
	adds="=";
      if (key==KEY_CHAR_RECIP)
	adds="inv";
      if (key==KEY_CHAR_SQUARE)
	adds="sq";
      if (key==KEY_CHAR_POWROOT)
	adds="surd";
      if (key==KEY_CHAR_CUBEROOT)
	adds="surd";
      if (key==KEY_CHAR_FACTOR)
	adds="factor";
      if (key==KEY_CHAR_NORMAL)
	adds="normal";
      int addssize=adds?strlen(adds):0;
      // cout << addssize << " " << adds << endl;
      if (key==KEY_CTRL_EXE){
	if (xcas::do_select(eq.data,true,value) && value.type==_EQW){
	  //cout << "ok " << value._EQWptr->g << endl;
	  //DefineStatusMessage((char*)lang?"resultat stocke dans last":"result stored in last", 1, 0, 0);
	  //DisplayStatusArea();
	  giac::sto(value._EQWptr->g,giac::gen("last",contextptr),contextptr);
	  return value._EQWptr->g;
	}
	//cout << "no " << eq.data << endl; if (value.type==_EQW) cout << value._EQWptr->g << endl ;
	return geq;
      }
      if ( key!=KEY_CHAR_MINUS && key!=KEY_CHAR_EQUAL &&
	   (ins || key==KEY_CHAR_PI || key==KEY_CTRL_VARS || (addssize==1 && (isalphanum(adds[0])|| adds[0]=='.' || adds[0]=='-') ) )
	   ){
	edited=true;
	if (line>=0 && xcas::eqw_select(eq.data,line,col,true,value)){
	  string s;
	  if (ins){
	    if (key==KEY_CTRL_PASTE)
	      s=paste_clipboard();
	    else {
	      if (value.type==_EQW){
		s=value._EQWptr->g.print(contextptr);
	      }
	      else
		s=value.print(contextptr);
	    }
	  }
	  else
	    s = adds;
	  string msg("Line ");
	  msg += print_INT_(line+1);
	  msg += " Col ";
	  msg += print_INT_(col+1);
	  if (inputline(msg.c_str(),0,s,false,65,contextptr)==KEY_CTRL_EXE){
	    value=gen(s,contextptr);
	    if (col<0)
	      (*geq._VECTptr)[line]=value;
	    else
	      (*((*geq._VECTptr)[line]._VECTptr))[col]=value;
	    redo=2;
	    key=KEY_SHIFT_RIGHT;
	  }
	  else
	    continue;
	}
	else {
	  vector<int> goto_sel;
	  if (xcas::Equation_adjust_xy(eq.data,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos,&goto_sel) && gsel && xcas::do_select(*gsel,true,value) && value.type==_EQW){
	    string s;
	    if (ins){
	      if (key==KEY_CTRL_PASTE)
		s=paste_clipboard();
	      else {
		s = value._EQWptr->g.print(contextptr);
		if (key==KEY_CHAR_COMMA)
		  s += ',';
	      }
	    }
	    else
	      s = adds;
	    if (inputline(value._EQWptr->g.print(contextptr).c_str(),0,s,false)==KEY_CTRL_EXE){
	      value=gen(s,contextptr);
	      //cout << value << " goto " << goto_sel << endl;
	      xcas::replace_selection(eq,value,gsel,&goto_sel,contextptr);
	      firstrun=-1; // workaround, force 2 times display
	    }
	    continue;
	  }
	}
      }
      if (redo){
	eq.data=0; // clear memory
	eq.data=xcas::Equation_compute_size(geq,eq.attr,LCD_WIDTH_PX,contextptr);
	eqdata=xcas::Equation_total_size(eq.data);
	if (redo==1){
	  dx=(eqdata.dx-LCD_WIDTH_PX)/2;
	  dy=LCD_HEIGHT_PX-2*margin+eqdata.y;
	  if (listormat) // select line l, col c
	    xcas::eqw_select(eq.data,line,col,true,value);
	  else {
	    xcas::Equation_select(eq.data,true);
	    xcas::eqw_select_down(eq.data);
	  }
	  continue;
	}
      }
      bool doit=eqdata.dx>=LCD_WIDTH_PX;
      int delta=0;
      if (listormat){
	if (key==KEY_CTRL_LEFT  || (!doit && key==KEY_SHIFT_LEFT)){
	  if (line>=0 && xcas::eqw_select(eq.data,line,col,false,value)){
	    if (col>=0){
	      --col;
	      if (col<0){
		col=ncols-1;
		if (line>0)
		  --line;
	      }
	    }
	    else {
	      if (line>0)
		--line;
	    }
	    xcas::eqw_select(eq.data,line,col,true,value);
	    if (doit) dx -= value._EQWptr->dx;
	  }
	  continue;
	}
	if (doit && key==KEY_SHIFT_LEFT){
	  dx -= 20;
	  continue;
	}
	if (key==KEY_CTRL_RIGHT  || (!doit && key==KEY_SHIFT_RIGHT)) {
	  if (line>=0 && xcas::eqw_select(eq.data,line,col,false,value)){
	    if (doit)
	      dx += value._EQWptr->dx;
	    if (col>=0){
	      ++col;
	      if (col==ncols){
		col=0;
		if (line<nlines-1)
		  ++line;
	      }
	    } else {
	      if (line<nlines-1)
		++line;
	    }
	    xcas::eqw_select(eq.data,line,col,true,value);
	  }
	  continue;
	}
	if (key==KEY_SHIFT_RIGHT && doit){
	  dx += 20;
	  continue;
	}
	doit=eqdata.dy>=LCD_HEIGHT_PX-2*margin;
	if (key==KEY_CTRL_UP || (!doit && key==KEY_CTRL_PAGEUP)){
	  if (line>0 && col>=0 && xcas::eqw_select(eq.data,line,col,false,value)){
	    --line;
	    xcas::eqw_select(eq.data,line,col,true,value);
	    if (doit)
	      dy += value._EQWptr->dy+eq.attr.fontsize/2;
	  }
	  continue;
	}
	if (key==KEY_CTRL_PAGEUP && doit){
	  dy += 10;
	  continue;
	}
	if (key==KEY_CTRL_DOWN  || (!doit && key==KEY_CTRL_PAGEDOWN)){
	  if (line<nlines-1 && col>=0 && xcas::eqw_select(eq.data,line,col,false,value)){
	    if (doit)
	      dy -= value._EQWptr->dy+eq.attr.fontsize/2;
	    ++line;
	    xcas::eqw_select(eq.data,line,col,true,value);
	  }
	  continue;
	}
	if ( key==KEY_CTRL_PAGEDOWN && doit){
	  dy -= 10;
	  continue;
	}
      }
      else { // else listormat
	if (key==KEY_CTRL_LEFT){
	  delta=xcas::eqw_select_leftright(eq,true,alph?2:0,contextptr);
	  // cout << "left " << delta << endl;
	  if (doit) dx += (delta?delta:-20);
	  continue;
	}
	if (key==KEY_SHIFT_LEFT){
	  delta=xcas::eqw_select_leftright(eq,true,1,contextptr);
	  vector<int> goto_sel;
	  if (doit) dx += (delta?delta:-20);
	  continue;
	}
	if (key==KEY_CTRL_RIGHT){
	  delta=xcas::eqw_select_leftright(eq,false,alph?2:0,contextptr);
	  // cout << "right " << delta << endl;
	  if (doit)
	    dx += (delta?delta:20);
	  continue;
	}
	if (key==KEY_SHIFT_RIGHT){
	  delta=xcas::eqw_select_leftright(eq,false,1,contextptr);
	  // cout << "right " << delta << endl;
	  if (doit)
	    dx += (delta?delta:20);
	  // dx=eqdata.dx-LCD_WIDTH_PX+20;
	  continue;
	}
	doit=eqdata.dy>=LCD_HEIGHT_PX-2*margin;
	if (key==KEY_CTRL_UP){
	  delta=xcas::eqw_select_up(eq.data);
	  // cout << "up " << delta << endl;
	  continue;
	}
	//cout << "up " << eq.data << endl;
	if (key==KEY_CTRL_PAGEUP && doit){
	  dy=eqdata.y+eqdata.dy+20;
	  continue;
	}
	if (key==KEY_CTRL_DOWN){
	  delta=xcas::eqw_select_down(eq.data);
	  // cout << "down " << delta << endl;
	  continue;
	}
	//cout << "down " << eq.data << endl;
	if ( key==KEY_CTRL_PAGEDOWN && doit){
	  dy=eqdata.y+LCD_HEIGHT_PX-margin;
	  continue;
	}
      }
      if (adds){
	edited=true;
	if (strcmp(adds,"'")==0)
	  adds="diff";
	if (strcmp(adds,"^2")==0)
	  adds="sq";
	if (strcmp(adds,">")==0)
	  adds="simplify";
	if (strcmp(adds,"<")==0)
	  adds="factor";
	if (strcmp(adds,"#")==0)
	  adds="partfrac";
	string cmd(adds);
	if (cmd.size() && cmd[cmd.size()-1]=='(')
	  cmd ='\''+cmd.substr(0,cmd.size()-1)+'\'';
	vector<int> goto_sel;
	if (xcas::Equation_adjust_xy(eq.data,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos,&goto_sel) && gsel){
	  gen op;
	  int addarg=0;
	  if (addssize==1){
	    switch (adds[0]){
	    case '+':
	      addarg=1;
	      op=at_plus;
	      break;
	    case '^':
	      addarg=1;
	      op=at_pow;
	      break;
	    case '=':
	      addarg=1;
	      op=at_equal;
	      break;
	    case '-':
	      addarg=1;
	      op=at_binary_minus;
	      break;
	    case '*':
	      addarg=1;
	      op=at_prod;
	      break;
	    case '/':
	      addarg=1;
	      op=at_division;
	      break;
	    case '\'':
	      addarg=1;
	      op=at_derive;
	      break;
	    }
	  }
	  if (op==0)
	    op=gen(cmd,contextptr);
	  if (op.type==_SYMB)
	    op=op._SYMBptr->sommet;
	  // cout << "keyed " << adds << " " << op << " " << op.type << endl;
	  if (op.type==_FUNC){
	    edited=true;
	    // execute command on selection
	    gen tmp,value;
	    if (xcas::do_select(*gsel,true,value) && value.type==_EQW){
	      if (op==at_integrate || op==at_sum)
		addarg=3;
	      if (op==at_limit)
		addarg=2;
	      gen args=eval(value._EQWptr->g,1,contextptr);
	      gen vx=xthetat?t__IDNT_e:x__IDNT_e;
	      if (addarg==1)
		args=makesequence(args,0);
	      if (addarg==2)
		args=makesequence(args,vx_var,0);
	      if (addarg==3)
		args=makesequence(args,vx_var,0,1);
	      if (op==at_surd)
		args=makesequence(args,key==KEY_CHAR_CUBEROOT?3:4);
	      if (op==at_subst)
		args=makesequence(args,giac::symb_equal(vx_var,0));
	      unary_function_ptr immediate_op[]={*at_eval,*at_evalf,*at_evalc,*at_regrouper,*at_simplify,*at_normal,*at_ratnormal,*at_factor,*at_cfactor,*at_partfrac,*at_cpartfrac,*at_expand,*at_canonical_form,*at_exp2trig,*at_trig2exp,*at_sincos,*at_lin,*at_tlin,*at_tcollect,*at_texpand,*at_trigexpand,*at_trigcos,*at_trigsin,*at_trigtan,*at_halftan};
	      if (equalposcomp(immediate_op,*op._FUNCptr)){
		set_abort();
		tmp=(*op._FUNCptr)(args,contextptr);
		clear_abort();
		esc_flag=0;
		giac::ctrl_c=false;
		giac::interrupted=false;
	      }
	      else
		tmp=symbolic(*op._FUNCptr,args);
	      //cout << "sel " << value._EQWptr->g << " " << tmp << " " << goto_sel << endl;
	      esc_flag=0;
	      giac::ctrl_c=false;
	      giac::interrupted=false;
	      if (!is_undef(tmp)){
		xcas::replace_selection(eq,tmp,gsel,&goto_sel,contextptr);
		if (addarg){
		  xcas::eqw_select_down(eq.data);
		  xcas::eqw_select_leftright(eq,false,0,contextptr);
		}
		eqdata=xcas::Equation_total_size(eq.data);
		dx=(eqdata.dx-LCD_WIDTH_PX)/2;
		dy=LCD_HEIGHT_PX-2*margin+eqdata.y;
		firstrun=-1; // workaround, force 2 times display
	      }
	    }
	  } // if (op.type==_FUNC)
	  
	} // if adjust_xy
      } // if (adds)
    }
    //*logptr(contextptr) << eq.data << endl;
  }

#ifndef NO_NAMESPACE_XCAS
} // namespace xcas
#endif // ndef NO_NAMESPACE_XCAS


#endif // NUMWORKS
