/******************************************************************/
/* KPresenter - (c) by Reginald Stadlbauer 1997-1999              */
/* Version: 0.1.0                                                 */
/* Author: Reginald Stadlbauer                                    */
/* E-Mail: reggie@kde.org                                         */
/* Homepage: http://boch35.kfunigraz.ac.at/~rs                    */
/* needs c++ library Qt (http://www.troll.no)                     */
/* needs mico (http://diamant.vsb.cs.uni-frankfurt.de/~mico/)     */
/* needs OpenParts and Kom (weis@kde.org)                         */
/* written for KDE (http://www.kde.org)                           */
/* License: GNU GPL                                               */
/******************************************************************/
/* Module: KPWebPresentation                                      */
/******************************************************************/

#ifndef webpresentation_h
#define webpresentation_h

#include <qstring.h>
#include <qstringlist.h>
#include <qcolor.h>
#include <qwizard.h>
#include <qdialog.h>

class KPresenterDoc;
class KPresenterView;
class Page;

class KColorButton;

class QVBox;
class QHBox;
class QLineEdit;
class QSpinBox;
class QComboBox;
class QListView;
class QListViewItem;
class QCloseEvent;
class QProgressBar;
class QLabel;

/******************************************************************/
/* Class: KPWebPresentation                                       */
/******************************************************************/

class KPWebPresentation
{
public:
    enum ImageFormat {BMP = 0, PNG, JPEG};

    static QString imageFormat( ImageFormat i ) {
        if ( i == BMP ) return QString( "bmp" );
        if ( i == PNG ) return QString( "png" );
        if ( i == JPEG ) return QString( "jpeg" );
        return QString::null;
    }

    KPWebPresentation( KPresenterDoc *_doc, KPresenterView *_view );
    KPWebPresentation( const QString &_config, KPresenterDoc *_doc, KPresenterView *_view );
    KPWebPresentation( const KPWebPresentation &webPres );

    void setAuthor( const QString &_author )
    { author = _author; }
    void setEMail( const QString &_email )
    { email = _email; }
    void setTitle( const QString &_title )
    { title = _title; }
    void setSlideTitles( const QStringList &_slideTitles )
    { slideTitles = _slideTitles; }
    void setBackColor( const QColor &_backColor )
    { backColor = _backColor; }
    void setTitleColor( const QColor &_titleColor )
    { titleColor = _titleColor; }
    void setTextColor( const QColor &_textColor )
    { textColor = _textColor; }
    void setImageFormat( ImageFormat _imgFormat )
    { imgFormat = _imgFormat; }
    void setPath( const QString &_path )
    { path = _path; }
    void setZoom( int _zoom )
    { zoom = _zoom; }

    QString getAuthor()
    { return author; }
    QString getEmail()
    { return email; }
    QString getTitle()
    { return title; }
    QStringList &getSlideTitles()
    { return slideTitles; }
    QColor getBackColor()
    { return backColor; }
    QColor getTitleColor()
    { return titleColor; }
    QColor getTextColor()
    { return textColor; }
    ImageFormat getImageFormat()
    { return imgFormat; }
    QString getPath()
    { return path; }
    int getZoom()
    { return zoom; }

    void setConfig( const QString &_config )
    { config = _config; }
    QString getConfig()
    { return config; }

    void loadConfig();
    void saveConfig();

    int initSteps() { return 7; }
    int slides1Steps() { return slideTitles.count(); }
    int slides2Steps() { return slideTitles.count(); }
    int mainSteps() { return 1; }

    void initCreation( QProgressBar *progressBar );
    void createSlidesPictures( QProgressBar *progressBar );
    void createSlidesHTML( QProgressBar *progressBar );
    void createMainPage( QProgressBar *progressBar );

protected:
    void init();
        
    KPresenterDoc *doc;
    KPresenterView *view;
    QString config;

    QString author, title, email;
    QStringList slideTitles;
    QColor backColor, titleColor, textColor;
    QString path;
    ImageFormat imgFormat;
    int zoom;

};

/******************************************************************/
/* Class: KPWebPresentationWizard                                 */
/******************************************************************/

class KPWebPresentationWizard : public QWizard
{
    Q_OBJECT

public:
    KPWebPresentationWizard( const QString &_config, KPresenterDoc *_doc, KPresenterView *_view );

    static void createWebPresentation( const QString &_config, KPresenterDoc *_doc, KPresenterView *_view );

protected:
    void setupPage1();
    void setupPage2();
    void setupPage3();

    bool isPathValid();

    void closeEvent( QCloseEvent *e );

    QString config;
    KPresenterDoc *doc;
    KPresenterView *view;
    KPWebPresentation webPres;

    QHBox *page1, *page2, *page3;
    QLineEdit *author, *title, *email, *path;
    KColorButton *textColor, *titleColor, *backColor;
    QComboBox *format;
    QSpinBox *zoom;
    QPushButton *choosePath;
    QListView *slideTitles;
    QLineEdit *slideTitle;

protected slots:
    virtual void finish();
    void slotChoosePath();
    void pageChanged();
    void slideTitleChanged( const QString & );
    void slideTitleChanged( QListViewItem * );

};

/******************************************************************/
/* Class: KPWebPresentationCreateDialog                           */
/******************************************************************/

class KPWebPresentationCreateDialog : public QDialog
{
    Q_OBJECT

public:
    KPWebPresentationCreateDialog( KPresenterDoc *_doc, KPresenterView *_view, const KPWebPresentation &_webPres );

    static void createWebPresentation( KPresenterDoc *_doc, KPresenterView *_view, const KPWebPresentation &_webPres );

    void start();

    void initCreation();
    void createSlidesPictures();
    void createSlidesHTML();
    void createMainPage();

protected:
    void setupGUI();
    void resizeEvent( QResizeEvent *e );

    KPresenterView *view;
    KPresenterDoc *doc;
    KPWebPresentation webPres;

    QProgressBar *progressBar;
    QLabel *step1, *step2, *step3, *step4;
    QPushButton *bDone, *bSave;
    QVBox *back;

protected slots:
    void saveConfig();

};
#endif

