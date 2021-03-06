#include "booksdlg.h"
#include <crgui.h>
#ifdef CR_POCKETBOOK
#include "cr3pocketbook.h"
#endif

#include <cri18n.h>
#include <dirent.h>    /* struct DIR, struct dirent, opendir().. */ 

#include "mainwnd.h"

static int endsWith(const char *str, const char *suffix) {
    if (!str || !suffix) {
        return 0;
    }
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr) {
        return 0;
    }
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

lString16 CRBookMenuItem::getBookPath() const {
    return _fBookPath;
}

static void walkDirRecursively(const char *dirname, CRBooksMenu* pParentMenu) {
    static size_t totalFound = 0;

    char fn[FILENAME_MAX];
    int len = strlen(dirname);
    strcpy(fn, dirname);
    fn[len++] = '/';

    struct dirent **entriesList;
    int n = scandir(dirname, &entriesList, 0, alphasort);
    if (n < 0) {
        CRLog::error("unable to perform scandir %s", dirname);
    } else {
        for (int i = 0; i < n; ++i) {	
            const char *fname = entriesList[i]->d_name;
            if (strcmp(fname, ".") == 0 || strcmp(fname, "..") == 0) {
                free(entriesList[i]);
                continue;
            } 

            strncpy(fn + len, fname, FILENAME_MAX - len);
            if ((entriesList[i]->d_type) == DT_REG) {
                if (endsWith(fname, ".epub") 
                    || endsWith(fname, ".fb2") 
                    || (endsWith(fname, ".txt") && !endsWith(fname, ".bmk.txt")) //skip bookmarks generated files 
                    || endsWith(fname, ".rtf")
                    || endsWith(fname, ".html")
                    || endsWith(fname, ".htm")
                    || endsWith(fname, ".pdb") 
                    || endsWith(fname, ".mobi")
                    || endsWith(fname, ".chm")
                    || endsWith(fname, ".fb2.zip")) {

                        fn[len + strlen(fname) + 1] = '\0';

                        CRLog::info("adding ebook file: %s", fn);
                        ++totalFound;

						pParentMenu->addItem(new CRBookMenuItem( pParentMenu, BOOKS_DIALOG_MENU_COMMANDS_START + totalFound, 
                            _(fname), LVImageSourceRef(), LVFontRef(), pParentMenu->getFont(), lString16(fn)));
                }
            } else if((entriesList[i]->d_type) == DT_DIR) {
                if (strcmp(fname, "dictionary") != 0 && strcmp(fname, "dictionaries") != 0 && !endsWith(fname, ".sdr")) { //skip kindle generated dummy folders and dictionaries folders as well
                    ++totalFound;

                    CRBooksMenu *pSubmenu = new CRBooksMenu(pParentMenu->getWindowManager(), pParentMenu, BOOKS_DIALOG_MENU_COMMANDS_START + totalFound,
                        _(fname),
                        LVImageSourceRef(), LVFontRef(), pParentMenu->getFont(), pParentMenu->getProps(), pParentMenu->getAccelerators(), pParentMenu->getDocview());

                    walkDirRecursively(fn, pSubmenu);

                    pSubmenu->reconfigure(0);

                    pParentMenu->addItem(pSubmenu);
                }
            }

            free(entriesList[i]);
        }

        free(entriesList);
    }

    pParentMenu->reconfigure(0);
}

CRBooksMenu::CRBooksMenu(CRGUIWindowManager * wm, CRMenu * parentMenu, int id, const char * label, LVImageSourceRef image, LVFontRef defFont, LVFontRef valueFont, CRPropRef props, CRGUIAcceleratorTableRef menuAccelerators, LVDocView *docview)
    :CRFullScreenMenu(wm, parentMenu, id, label, image, defFont, valueFont, props),
    _valueFont(LVFontRef(fontMan->GetFont( VALUE_FONT_SIZE, 400, true, css_ff_sans_serif, lString8("Arial")))),
    _menuAccelerators(menuAccelerators),
    _docview(docview)
    {
        setSkinName(lString16(L"#settings"));
        setAccelerators( _menuAccelerators );
        _fullscreen = true;
        reconfigure(0);
    }

bool CRBooksMenu::onCommand( int command, int params ) {

    if ( command >= MCMD_SELECT_1 && command <= MCMD_SELECT_9 ) {
        const int index = command - MCMD_SELECT_1 + getTopItem();
        
        if (index < 0 || index >= _items.length()) {
            CRLog::error( "CRBooksMenu::onItemSelect() - invalid selection: %d", index);
            return true;
        }

        CRMenuItem *item = getItems()[index];

        if (item->isSubmenu()) {
            _wm->activateWindow((CRMenu*) item);
        } else {
            CRBookMenuItem *bookMenuItem = static_cast<CRBookMenuItem *>(item); 
            highlightCommandItem( bookMenuItem->getId() ); 
            _docview->close(); 
            const lString16 &fullBookPath = bookMenuItem->getBookPath(); 
            if ( !_docview->LoadDocument( fullBookPath.c_str() ) ) {
                CRLog::error("CRBooksMenu::onCommand( %s ) - failed!", fullBookPath.c_str());
            }
            _docview->restorePosition();
            closeAllMenu(0, 0);
        }
        return true;
    } else {
        return CRMenu::onCommand( command, params );	
    }
}

CRMenu *createBooksDialogMenu(const char *path, CRGUIWindowManager * wm, LVFontRef font, CRPropRef props, CRGUIAcceleratorTableRef menuAccelerators, LVDocView *docview) {
    CRBooksMenu *pResultMenu = new CRBooksMenu(wm, NULL, MCMD_MAIN_MENU, _("Select book"), LVImageSourceRef(), LVFontRef(), font, props, menuAccelerators, docview);
    walkDirRecursively(path, pResultMenu);
    return pResultMenu;
}
