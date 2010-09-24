
#include <QWidget>

class KoDocument;

class KoApplicationHelper
{
public:
    KoApplicationHelper();
    virtual ~KoApplicationHelper();

protected:
    enum MessageType {
        UnsupportedFileTypeMessage,
        InformationMessage,
        InformationWithTimeoutMessage
    };

    /*!
     * Shows message with text @a messageText of type @a type.
     */
    virtual void showMessage(MessageType type, const QString& messageText = QString()) = 0;

    /*!
     * Starts new instance of the application and opens document from @a fileName.
     * If @a isNewDocument is true, new document is created.
     */
    virtual void startNewInstance(const QString& fileName, bool isNewDocument) = 0;

    /*!
     * Shows or hides progress bar indicator.
     */
    virtual void setProgressIndicatorVisible(bool visible) = 0;

    /*!
     * Performs UI initialization on document opening.
     */
    virtual void showUiOnDocumentOpening() = 0;

    /*!
     * @return true if @a fileName is supported by the application
     */
    bool isFileSupported(const QString& fileName) const;

    /*!
     * Opens document from @a fileName. If @a isNewDocument is true, new document is created.
     */
    bool openDocument(const QString &fileName, bool isNewDocument);

    /*!
     * Opened file
     */
    QString m_fileName;
    /*!
     * Pointer to KoDocument
     */
    KoDocument *m_doc;
    /*!
     * true if document is currently being loaded
     */
    bool m_isLoading;

    FoCellToolFactory *m_cellToolFactory;
};
