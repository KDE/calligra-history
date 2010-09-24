
#include "KoApplicationHelper.h"
#include "Common.h"
#include "FoCellToolFactory.h"

#include <KMimeType>
#include <KLocale>

#include <QFile>
#include <QtDebug>

KoApplicationHelper::KoApplicationHelper()
    : m_doc(0),
      m_isLoading(false)
{
}

KoApplicationHelper::~KoApplicationHelper()
{
}

bool KoApplicationHelper::isFileSupported(const QString& fileName) const
{
    QList<QString> extensions;

    //Add Txt extension after adding ascii filter to koffice package
    /*extensions << EXT_DOC << EXT_DOCX << EXT_ODT << EXT_TXT \*/
    extensions << EXT_DOC << EXT_DOCX << EXT_ODT << EXT_TXT
        << EXT_PPT << EXT_PPTX << EXT_ODP << EXT_PPS << EXT_PPSX
        << EXT_ODS << EXT_XLS << EXT_XLSX;

    const QString ext = KMimeType::extractKnownExtension(fileName);
    if (extensions.contains(ext))
        return true;

    return false;
}

bool KoApplicationHelper::openDocument(const QString &fileName, bool isNewDocument)
{
    if (m_type == Presentation) {
        m_ui->actionInsertTextShape->setVisible(true);
        m_ui->actionInsertTable->setVisible(true);
    }

    //check if the file exists
    if(!QFile(fileName).exists()) {
        showMessage(InformationWithTimeoutMessage, i18n("No such file exists"));
        return false;
    }

    //check if the format is supported for opening
    if (!isFileSupported(fileName)) {
        showMessage(UnsupportedFileTypeMessage);
        qDebug() << "Currently this file format is not supported";
        return false;
    }

    //if the current instance has a document open start a new one
    if (m_doc) {
        startNewInstance(fileName, isNewDocument);
        return true;
    }

    showUiOnDocumentOpening();

    setProgressIndicatorVisible(true);
    QString mimetype = KMimeType::findByPath(fileName)->name();
    int errorCode = 0;
    m_isLoading = true;

    m_doc = KParts::ComponentFactory::createPartInstanceFromQuery<KoDocument>(
                mimetype, QString(),
                0, 0, QStringList(),
                &errorCode);

    if (!m_doc) {
        setProgressIndicatorVisible(false);
        return false;
    }

    //for new files the condition to be checked is m_fileName.isEmpty()
    if(isNewDocument) {
        m_fileName.clear();
    } else {
        m_fileName=fileName;
    }

    KUrl fileUrl;
    fileUrl.setPath(fileName);

    m_doc->setCheckAutoSaveFile(false);
    m_doc->setAutoErrorHandlingEnabled(true);

    //actual opening of the document happens here.
    if (!m_doc->openUrl(fileUrl)) {
        setProgressIndicatorVisible(false);
        return false;
    }

    m_doc->setReadWrite(true);
    m_doc->setAutoSave(0);

    // registering tools
    m_cellToolFactory=new FoCellToolFactory(KoToolRegistry::instance());
   // m_spreadEdit->setCellTool(m_focelltool);
    KoToolRegistry::instance()->add(m_cellToolFactory);

    m_view = m_doc->createView();
    QList<KoCanvasControllerWidget*> controllers = m_view->findChildren<KoCanvasControllerWidget*>();
    if (controllers.isEmpty()) {
        setProgressIndicatorVisible(false);
        return false; // Panic
    }

    m_controller = controllers.first();

    if (!m_controller) {
        setProgressIndicatorVisible(false);
        return false;
    }

    QString fname = fileUrl.fileName();
    QString ext = KMimeType::extractKnownExtension(fname);
    if (ext.length()) {
        fname.chop(ext.length() + 1);
    }

    //file type of the document
    if (!QString::compare(ext, EXT_ODP, Qt::CaseInsensitive) ||
            !QString::compare(ext, EXT_PPS, Qt::CaseInsensitive) ||
            !QString::compare(ext, EXT_PPSX, Qt::CaseInsensitive) ||
            !QString::compare(ext, EXT_PPTX, Qt::CaseInsensitive) ||
            !QString::compare(ext, EXT_PPT, Qt::CaseInsensitive))
    {
        m_type = Presentation;
    } else if (!QString::compare(ext, EXT_ODS, Qt::CaseInsensitive) ||
               !QString::compare(ext, EXT_XLSX, Qt::CaseInsensitive) ||
               !QString::compare(ext, EXT_XLS, Qt::CaseInsensitive))
    {
        m_type = Spreadsheet;
    } else {
        m_type = Text;
        // We need to get the page count again after layout rounds.
        connect(m_doc, SIGNAL(pageSetupChanged()), this, SLOT(updateUI()));
    }

    m_ui->actionMathOp->setVisible(false);
    m_ui->actionFormat->setVisible(false);
    m_ui->actionSlidingMotion->setVisible(false);

    if (m_type == Spreadsheet) {
        m_ui->actionEdit->setVisible(true);
        KoToolManager::instance()->addController(m_controller);
        QApplication::sendEvent(m_view, new KParts::GUIActivateEvent(true));
        m_focelltool = dynamic_cast<FoCellTool *>(KoToolManager::instance()->toolById(((View*)m_view)->selection()->canvas(),CellTool_ID));
        ((View *)m_view)->showTabBar(false);
        ((View *)m_view)->setStyleSheet("* { color:white; } ");

        //set the central widget. Note: The central widget for spreadsheet is m_view.
        setCentralWidget(((View *)m_view));
        setUpSpreadEditorToolBar();
    } else if((m_type==Text) && ((!QString::compare(ext,EXT_ODT,Qt::CaseInsensitive)) ||
                                (!QString::compare(ext,EXT_DOC,Qt::CaseInsensitive)) ||
                                (!QString::compare(ext,EXT_DOCX,Qt::CaseInsensitive)))) {
        m_kwview = qobject_cast<KWView *>(m_view);
        m_editor = qobject_cast<KoTextEditor *>(m_kwview->canvasBase()->toolProxy()->selection());
        m_ui->actionFormat->setVisible(true);
        m_ui->actionEdit->setVisible(true);

        //set the central widget here
        setCentralWidget(m_controller);
    } else if(m_type == Presentation) {
        m_ui->actionFormat->setVisible(true);
        m_ui->actionEdit->setVisible(true);
        m_ui->actionSlidingMotion->setVisible(true);
        //set the central widget here
        setCentralWidget(m_controller);

        //code related to the button previews
        if(storeButtonPreview!=0) {
            disconnect(storeButtonPreview,SIGNAL(gotoPage(int)),this,SLOT(gotoPage(int)));
            delete storeButtonPreview;
        }
        storeButtonPreview=new StoreButtonPreview(m_doc,m_view);
        connect(storeButtonPreview,SIGNAL(gotoPage(int)),this,SLOT(gotoPage(int)));
    } else {
        //condition required for plain text document that is opened.
        setCentralWidget(m_controller);
    }

    setWindowTitle(QString("%1 - %2").arg(i18n("FreOffice"), fname));

    m_controller->setProperty("FingerScrollable", true);

    QTimer::singleShot(250, this, SLOT(updateUI()));

    KoCanvasBase *canvas = m_controller->canvas();
    connect(canvas->resourceManager(),
            SIGNAL(resourceChanged(int, const QVariant &)),
            this,
            SLOT(resourceChanged(int, const QVariant &)));
    connect(KoToolManager::instance(),
            SIGNAL(changedTool(KoCanvasController*, int)),
            SLOT(activeToolChanged(KoCanvasController*, int)));

    setProgressIndicatorVisible(false);
    m_isLoading = false;

    if (m_splash && !this->isActiveWindow()) {
        this->show();
        m_splash->finish(m_controller);
        m_splash = 0;
   }

   //This is a hack for the uppercase problem.
   m_firstChar=true;

   //When the user opens a new document it should open in the edit mode directly
   if(isNewDocument) {
       //This timer is required, otherwise a new spread sheet will
       //have an unwanted widget in the view.
       QTimer::singleShot(1,this,SLOT(editToolBar()));
       if(m_type==Text) {
           centralWidget()->setInputMethodHints(Qt::ImhNoAutoUppercase);
       }
   } else {
       //activate the pan tool.
       editToolBar(false);
   }

   m_ui->actionSearch->setChecked(false);
   m_undostack = m_doc->undoStack();
   if(m_type==Text && !isNewDocument){
       digitalSignatureDialog=new DigitalSignatureDialog(this->m_fileName);
        if(digitalSignatureDialog->verifySignature()){
            QMenuBar* menu = menuBar();
            menu->insertAction(m_ui->actionClose,m_ui->actionDigitalSignature);
            connect(m_ui->actionDigitalSignature,SIGNAL(triggered()),this,SLOT(showDigitalSignatureInfo()));
        }
   }
#ifdef Q_WS_MAEMO_5
   if(m_type==Text)
   {
       /*
        * intialising the rdf
        */
       KoStore::Backend backend = KoStore::Auto;
       KoStore * store = KoStore::createStore(fileName, KoStore::Read, "", backend);

       foDocumentRdf=new FoDocumentRdf(m_doc,m_editor);
       if (!foDocumentRdf->loadOasis(store)) {
           delete foDocumentRdf;
           foDocumentRdf = 0;
           return false;
       }
       rdfShortcut= new QShortcut(QKeySequence(("Ctrl+R")),this);
       connect(rdfShortcut,SIGNAL(activated()),foDocumentRdf,SLOT(highlightRdf()));
   }
#endif
    return true;
}
