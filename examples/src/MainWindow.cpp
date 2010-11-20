#include "MainWindow.h"

#include <Core/pSettings>
#include <Core/pVersion>
#include <Gui/pDockToolBar>
#include <Gui/pActionsNodeModel>
#include <Gui/pActionsNodeShortcutEditor>
#include <Gui/pActionsNodeMenuBar>
#include <Gui/pStringListEditor>
#include <Gui/pFileListEditor>
#include <Gui/pPathListEditor>
#include <Gui/pColorButton>
#include <Gui/pDrawingUtils>
#include <Gui/pToolButton>
#include <Gui/pIconManager>
#include <Gui/pQueuedMessageToolBar>
#include <Gui/pStylesActionGroup>
#include <Gui/pStylesToolButton>
#include <Gui/pFileDialog>
#include <Gui/pTreeComboBox>
#include <Gui/pConsole>
#include <Gui/pConsoleCommand>

#if defined( QT_MODELTEST )
#include <modeltest.h>
#endif

#include <QtGui>

class ConsoleCommands : public pConsoleCommand
{
public:
	ConsoleCommands()
		: pConsoleCommand( QStringList() << "ls" << "echo" << "quit" )
	{
		setUsage( "ls", pConsole::tr( "Execute 'dir' on windows, else 'ls' command and return output.\nExample: ls -C /" ) );
		setUsage( "echo", pConsole::tr( "Simple echo command, print back the arguments.\nExample: echo \"hi all friends\"" ) );
		setUsage( "quit", pConsole::tr( "Quit the application.\nExample: quit" ) );
	}
	
	virtual QString interpret( const QString& command, int* exitCode ) const
	{
		int ec = pConsoleCommand::NotFound;
		QString output = pConsoleCommand::interpret( command, &ec );
		QStringList parts = parseCommand( command );
		const QString cmd = parts.isEmpty() ? QString::null : parts.takeFirst();
		
		if ( ec != pConsoleCommand::NotFound ) {
			// nothing to do
		}
		else if ( cmd == "ls" ) {
#if defined ( Q_OS_WIN )
			const QString cmd = QString( "dir %1" ).arg( pConsoleCommand::quotedStringList( parts ).join( " " ) ).trimmed();
#else
			const QString cmd = QString( "ls %1" ).arg( pConsoleCommand::quotedStringList( parts ).join( " " ) ).trimmed();
#endif
			
			QProcess process;
			process.setProcessChannelMode( QProcess::MergedChannels );
			process.start( cmd );
			process.waitForStarted();
			process.waitForFinished();
			
			output = QString::fromLocal8Bit( process.readAll().trimmed() );
			ec = process.exitCode();
		}
		else if ( cmd == "echo" ) {
			output = parts.isEmpty() ? pConsole::tr( "No argument given" ) : parts.join( "\n" );
			ec = parts.isEmpty() ? pConsoleCommand::Error : pConsoleCommand::Success;
		}
		else if ( cmd == "quit" ) {
			output = pConsole::tr( "Quitting the application..." );
			ec = pConsoleCommand::Success;
			
			QTimer::singleShot( 1000, qApp, SLOT( quit() ) );
		}
		
		if ( exitCode ) {
			*exitCode = ec;
		}
		
		return output;
	}
};

MainWindow::MainWindow( QWidget* parent )
	: pMainWindow( parent )
{
	initializeGui();
}

MainWindow::~MainWindow()
{
}

void MainWindow::saveState()
{
	pMainWindow::saveState();
	
	settings()->setValue( "MainWindow/Style", agStyles->currentStyle() );
}

void MainWindow::restoreState()
{
	pMainWindow::restoreState();
	
	agStyles->setCurrentStyle( settings()->value( "MainWindow/Style" ).toString() );
}

void MainWindow::initializeGui()
{
	twPages = new QTabWidget( this );
	setCentralWidget( twPages );
	
	// initialize gui
	initializeMenuBar();
	pteLog = initializePlainTextEdit();
	tvActions = initializeActionsTreeView();
	cShell = initializeConsole();
	versionsTests();
	createListEditors();
	createCustomWidgets();
	
	// create fake dock widget for testing
	/*for ( int i = 0; i < 10; i++ ) {
		QDockWidget* dw = new QDockWidget( this );
		dw->setObjectName( QString( "Dock%1" ).arg( i ) );
		dw->toggleViewAction()->setObjectName( QString( "DockViewAction%1" ).arg( i ) );
		dockToolBar( Qt::LeftToolBarArea )->addDockWidget( dw, QString( "Dock %1" ).arg( i ), QIcon( scaledPixmap( ":/fresh/country-flags/ad.png", QSize( 96, 96 ) ) ) );
	}*/
	
	// list dock widgets in the menu
	foreach ( QDockWidget* dockWidget, findChildren<QDockWidget*>() ) {
		//dockWidget->setAttribute( Qt::WA_DeleteOnClose );
		QAction* action = dockWidget->toggleViewAction();
		menuBar()->addAction( QString( "mView/mDockWidgets/%1" ).arg( action->objectName() ), action );
		pActionsNode node = mActionsModel->actionToNode( action );
		node.setDefaultShortcut( QKeySequence( QString( "Ctrl+%1" ).arg( QChar( qMax( 32, qrand() % 127 ) ) ) ) );
	}
}

void MainWindow::initializeMenuBar()
{
	// set menu bar model
	mActionsModel = new pActionsNodeModel( this );
	menuBar()->setModel( mActionsModel );
	
#if defined( QT_MODELTEST )
	new ModelTest( mActionsModel, this );
#endif
	
	// create menus and sub menus
	menuBar()->addMenu( "mFile" ).setText( tr( "&File" ) );
	menuBar()->addMenu( "mEdit" ).setText( tr( "&Edit" ) );
	menuBar()->addMenu( "mView" ).setText( tr( "&View" ) );
	menuBar()->addMenu( "mView/mStyle" ).setText( tr( "&Style" ) );
	menuBar()->addMenu( "mView/mMode" ).setText( tr( "&Mode" ) );
	menuBar()->addMenu( "mView/mDockToolBarManager" ).setText( tr( "&Dock ToolBar Manager" ) );
	menuBar()->addMenu( "mView/mDockWidgets" ).setText( tr( "Dock &Widgets" ) );
	
	// create actions
	QAction* aQuit = menuBar()->addAction( "mFile/aQuit", tr( "&Quit" ) );
	
	QAction* aClassic = menuBar()->addAction( "mView/mMode/aShowClassic", tr( "Classic" ) );
	aClassic->setCheckable( true );
	
	QAction* aModern = menuBar()->addAction( "mView/mMode/aShowModern", tr( "Modern" ) );
	aModern->setCheckable( true );
	
	// style actions
	agStyles = new pStylesActionGroup( this );
	agStyles->installInMenuBar( menuBar(), "mView/mStyle" );
	
	// action group
	QActionGroup* agDockToolBarManagerMode = new QActionGroup( this );
	agDockToolBarManagerMode->addAction( aClassic );
	agDockToolBarManagerMode->addAction( aModern );
	
	// add dock toolbar manager actions
	foreach ( pDockToolBar* dockToolBar, dockToolBarManager()->dockToolBars() ) {
		QAction* action;
		
		action = dockToolBar->toggleViewAction();
		menuBar()->addAction( QString( "mView/mDockToolBarManager/%1" ).arg( action->objectName() ), action );
		
		action = dockToolBar->toggleExclusiveAction();
		menuBar()->addAction( QString( "mView/mDockToolBarManager/%1" ).arg( action->objectName() ), action );
	}
	
	// connections
	connect( aQuit, SIGNAL( triggered() ), this, SLOT( close() ) );
	connect( agStyles, SIGNAL( styleSelected( const QString& ) ), this, SLOT( setCurrentStyle( const QString& ) ) );
	connect( dockToolBarManager(), SIGNAL( modeChanged( pDockToolBarManager::Mode ) ), this, SLOT( dockToolBarManagerModeChanged( pDockToolBarManager::Mode ) ) );
	connect( aClassic, SIGNAL( triggered() ), this, SLOT( dockToolBarManagerClassic() ) );
	connect( aModern, SIGNAL( triggered() ), this, SLOT( dockToolBarManagerModern() ) );
}

QPlainTextEdit* MainWindow::initializePlainTextEdit()
{
	QPlainTextEdit* pte = new QPlainTextEdit( this );
	pte->setReadOnly( true );
	pte->setTabStopWidth( 40 );
	twPages->addTab( pte, tr( "Log" ) );
	return pte;
}

QTreeView* MainWindow::initializeActionsTreeView()
{
	QTreeView* tv = new QTreeView( this );
	tv->setModel( menuBar()->model() );
	
	menuBar()->addMenu( "mEdit/mActions" ).setText( tr( "&Actions" ) );
	
	QAction* aAddAction = menuBar()->addAction( "mEdit/mActions/aAddAction", tr( "&Add new action" ) );
	QAction* aRemoveAction = menuBar()->addAction( "mEdit/mActions/aRemoveAction", tr( "&Remove selected action" ) );
	QAction* aEditTextNode = menuBar()->addAction( "mEdit/mActions/aEditTextNode", tr( "&Edit selected node text" ) );
	QAction* aEditShortcuts = menuBar()->addAction( "mEdit/mActions/aEditShortcuts", tr( "Edit the actions &shortcuts" ) );
	
	connect( aAddAction, SIGNAL( triggered() ), this, SLOT( aAddAction_triggered() ) );
	connect( aRemoveAction, SIGNAL( triggered() ), this, SLOT( aRemoveAction_triggered() ) );
	connect( aEditTextNode, SIGNAL( triggered() ), this, SLOT( aEditTextNode_triggered() ) );
	connect( aEditShortcuts, SIGNAL( triggered() ), this, SLOT( aEditShortcuts_triggered() ) );
	
	twPages->addTab( tv, tr( "Actions" ) );
	
	return tv;
}

pConsole* MainWindow::initializeConsole()
{
	pConsole* shell = new pConsole( this );
	ConsoleCommands* commands = new ConsoleCommands;
	shell->addAvailableCommand( commands );
	
	QDockWidget* dwConsole = new QDockWidget( this );
	dwConsole->setObjectName( "Console" );
	dwConsole->setWidget( shell );
	dwConsole->toggleViewAction()->setObjectName( "ConsoleViewAction" );
	dockToolBar( Qt::BottomToolBarArea )->addDockWidget( dwConsole, tr( "Shell" ), QIcon( pDrawingUtils::scaledPixmap( ":/fresh/country-flags/ro.png", QSize( 96, 96 ) ) ) );
	
	return shell;
}

void MainWindow::versionsTests()
{
	const pVersion v1( "1.5.4" );
	const pVersion v2( "1.5.4a" );
	const QString test1 = QString(
			"%1\t>\t%2:\t%3\n"
		"%4\t<\t%5:\t%6\n"
		"%7\t>=\t%8:\t%9\n"
		"%10\t<=\t%11:\t%12\n"
		"%13\t==\t%14:\t%15\n"
		"%16\t!=\t%17:\t%18"
		)
		.arg( v1.toString() ).arg( v2.toString() ).arg( v1 > v2 )
		.arg( v1.toString() ).arg( v2.toString() ).arg( v1 < v2 )
		.arg( v1.toString() ).arg( v2.toString() ).arg( v1 >= v2 )
		.arg( v1.toString() ).arg( v2.toString() ).arg( v1 <= v2 )
		.arg( v1.toString() ).arg( v2.toString() ).arg( v1 == v2 )
		.arg( v1.toString() ).arg( v2.toString() ).arg( v1 != v2 )
		;
	const QString test2 = QString(
			"%1\t>\t%2:\t%3\n"
			"%4\t<\t%5:\t%6\n"
			"%7\t>=\t%8:\t%9\n"
			"%10\t<=\t%11:\t%12\n"
			"%13\t==\t%14:\t%15\n"
			"%16\t!=\t%17:\t%18"
		)
		.arg( v1.toString() ).arg( v1.toString() ).arg( v1 > v1 )
		.arg( v1.toString() ).arg( v1.toString() ).arg( v1 < v1 )
		.arg( v1.toString() ).arg( v1.toString() ).arg( v1 >= v1 )
		.arg( v1.toString() ).arg( v1.toString() ).arg( v1 <= v1 )
		.arg( v1.toString() ).arg( v1.toString() ).arg( v1 == v1 )
		.arg( v1.toString() ).arg( v1.toString() ).arg( v1 != v1 )
		;
	
	pteLog->appendPlainText( tr( "Testing versions %1 & %2:\n" ).arg( v1.toString() ).arg( v2.toString() ) );
	pteLog->appendPlainText( tr( "Test 1:\n%1\n" ).arg( test1 ) );
	pteLog->appendPlainText( tr( "Test 2:\n%1" ).arg( test2 ) );
}

void MainWindow::createListEditors()
{
	// list editor dock
	QDockWidget* dwListEditor = new QDockWidget( this );
	QTabWidget* twListEditors = new QTabWidget( dwListEditor );
	dwListEditor->setObjectName( "DockListEditor" );
	dwListEditor->setWidget( twListEditors );
	dwListEditor->toggleViewAction()->setObjectName( "DockListEditorViewAction" );
	dockToolBar( Qt::LeftToolBarArea )->addDockWidget( dwListEditor, tr( "List Editor" ), QIcon( pDrawingUtils::scaledPixmap( ":/fresh/country-flags/fr.png", QSize( 96, 96 ) ) ) );
	
	twListEditors->addTab( new pStringListEditor( QString::null, this ), tr( "Edit strings" ) );
	twListEditors->addTab( new pPathListEditor( QString::null, ".", this ), tr( "Edit paths" ) );
	twListEditors->addTab( new pFileListEditor( QString::null, ".", "*.png", this ), tr( "Edit files" ) );
}

void MainWindow::createCustomWidgets()
{
	QDockWidget* dwWidgets = new QDockWidget( this );
	dwWidgets->setObjectName( "dwWidgets" );
	dwWidgets->toggleViewAction()->setObjectName( "dwWidgetsViewAction" );
	dockToolBar( Qt::LeftToolBarArea )->addDockWidget( dwWidgets, tr( "Custom Widgets" ), QIcon( pDrawingUtils::scaledPixmap( ":/fresh/country-flags/es.png", QSize( 96, 96 ) ) ) );
	
	QWidget* dwWidgetsContents = new QWidget( this );
	QGridLayout* dwWidgetsContentsLayout = new QGridLayout( dwWidgetsContents );
	dwWidgets->setWidget( dwWidgetsContents );
	
	pColorButton* colorButton = new pColorButton( dwWidgetsContents );
	dwWidgetsContentsLayout->addWidget( colorButton, 0, 0 );
	
	pToolButton* toolButton1 = new pToolButton( dwWidgetsContents );
	toolButton1->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
	toolButton1->setText( tr( "Bottom To Top" ) );
	toolButton1->setIcon( pIconManager::icon( "pt.png" ) );
	toolButton1->setDirection( QBoxLayout::BottomToTop );
	dwWidgetsContentsLayout->addWidget( toolButton1, 0, 1 );
	
	pToolButton* toolButton2 = new pToolButton( dwWidgetsContents );
	toolButton2->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
	toolButton2->setText( tr( "Top To Bottom" ) );
	toolButton2->setIcon( pIconManager::icon( "br.png" ) );
	toolButton2->setDirection( QBoxLayout::TopToBottom );
	dwWidgetsContentsLayout->addWidget( toolButton2, 0, 2 );
	
	QPushButton* pbOpenFile = new QPushButton( tr( "Get open file names" ), this );
	dwWidgetsContentsLayout->addWidget( pbOpenFile, 7, 0, 1, 3 );
	connect( pbOpenFile, SIGNAL( clicked() ), this, SLOT( openFileDialog() ) );
	
	QPushButton* pbOpenDirectory = new QPushButton( tr( "Get open directory name" ), this );
	dwWidgetsContentsLayout->addWidget( pbOpenDirectory, 8, 0, 1, 3 );
	connect( pbOpenDirectory, SIGNAL( clicked() ), this, SLOT( openDirectoryDialog() ) );
	
	QPushButton* pbQueuedMessage = new QPushButton( tr( "Add queued message" ) );
	dwWidgetsContentsLayout->addWidget( pbQueuedMessage, 9, 0, 1, 3 );
	connect( pbQueuedMessage, SIGNAL( clicked() ), this, SLOT( addQueuedMessage() ) );
	
	pStylesToolButton* stylesButton = new pStylesToolButton( dwWidgetsContents );
	stylesButton->setSizePolicy( pbQueuedMessage->sizePolicy() );
	stylesButton->setCheckableActions( false );
	dwWidgetsContentsLayout->addWidget( stylesButton, 10, 0, 1, 3 );
	connect( stylesButton, SIGNAL( styleSelected( const QString& ) ), agStyles, SLOT( setCurrentStyle( const QString& ) ) );
	
	pTreeComboBox* tcbActions = new pTreeComboBox( this );
	tcbActions->setModel( mActionsModel );
	dwWidgetsContentsLayout->addWidget( tcbActions, 11, 0, 1, 3 );
}

void MainWindow::aAddAction_triggered()
{
	const QModelIndex index = tvActions->selectionModel()->selectedIndexes().value( 0 );
	QString path;
	
	if ( index.isValid() ) {
		const pActionsNode node = mActionsModel->indexToNode( index );
		
		if ( node.type() == pActionsNode::Action ) {
			path = node.path().section( '/', 0, -2 );
		}
		else {
			path = node.path();
		}
	}
	
	if ( !path.isEmpty() ) {
		path.append( '/' );
	}
	
	path = QInputDialog::getText( this, QString::null, tr( "Enter the full path where to add the action (/some/path/to/add/the/actionName):" ), QLineEdit::Normal, path );
	
	if ( path.count( "/" ) == 0 || QString( path ).replace( "/", QString::null ).trimmed().isEmpty() ) {
		return;
	}
	
	QAction* action = new QAction( this );
	action->setText( path.section( '/', -1, -1 ) );
	
	if ( !mActionsModel->addAction( path, action ) ) {
		delete action;
		QMessageBox::information( this, QString::null, tr( "Can't add action to '%1'" ).arg( path ) );
	}
}

void MainWindow::aRemoveAction_triggered()
{
	const QModelIndex index = tvActions->selectionModel()->selectedIndexes().value( 0 );
	
	if ( index.isValid() ) {
		const pActionsNode node = mActionsModel->indexToNode( index );
		
		if ( !mActionsModel->removeAction( node.path() ) ) {
			QMessageBox::information( this, QString::null, tr( "Can't remove action '%1'" ).arg( node.path() ) );
		}
	}
}

void MainWindow::aEditTextNode_triggered()
{
	const QModelIndex index = tvActions->selectionModel()->selectedIndexes().value( 0 );
	
	if ( index.isValid() ) {
		pActionsNode node = mActionsModel->indexToNode( index );
		const QString text = QInputDialog::getText( this, QString::null, tr( "Enter the new node text:" ), QLineEdit::Normal, node.text() );
		
		if ( !text.isEmpty() ) {
			node.setText( text );
		}
	}
}

void MainWindow::aEditShortcuts_triggered()
{
	pActionsNodeShortcutEditor dlg( mActionsModel, this );
	dlg.exec();
}

void MainWindow::dockToolBarManagerModeChanged( pDockToolBarManager::Mode mode )
{
	switch ( mode ) {
		case pDockToolBarManager::Classic:
			menuBar()->action( "mView/mMode/aShowClassic" )->trigger();
			break;
		case pDockToolBarManager::Modern:
			menuBar()->action( "mView/mMode/aShowModern" )->trigger();
			break;
		default:
			break;
	}
}

void MainWindow::dockToolBarManagerClassic()
{
	dockToolBarManager()->setMode( pDockToolBarManager::Classic );
}

void MainWindow::dockToolBarManagerModern()
{
	dockToolBarManager()->setMode( pDockToolBarManager::Modern );
}

void MainWindow::addQueuedMessage()
{
	bool ok;
	const QString message = QInputDialog::getText( this, tr( "Add a queued message" ), tr( "Enter the message to show:" ), QLineEdit::Normal, tr( "This is the default message" ), &ok );
	
	if ( ok && !message.isEmpty() ) {
		pQueuedMessage msg;
		msg.message = message;
		msg.buttons[ QDialogButtonBox::Ok ] = QString::null;
		msg.buttons[ QDialogButtonBox::Yes ] = tr( "Show QMessageBox" );
		msg.object = this;
		msg.slot = "queuedMessageToolBarButtonClicked";
		
		queuedMessageToolBar()->appendMessage( msg );
	}
}

void MainWindow::queuedMessageToolBarButtonClicked( QDialogButtonBox::StandardButton button, const pQueuedMessage& message )
{
	switch ( button ) {
		case QDialogButtonBox::Yes:
			QMessageBox::information( this, QString::null, message.message );
			break;
		default:
			break;
	}
}

void MainWindow::setCurrentStyle( const QString& style )
{
	QApplication::setStyle( style );
}

void MainWindow::openFileDialog()
{
	const QString caption;
	const QString dir;
	const QString filter;
	const bool enabledTextCodec = true;
	const bool enabledOpenReadOnly = true;
	const QString selectedFilter;
	const QFileDialog::Options options = 0;
	const pFileDialogResult result = pFileDialog::getOpenFileNames( this, caption, dir, filter, enabledTextCodec,
		enabledOpenReadOnly, selectedFilter, options );
	
	pteLog->appendPlainText( QString::null );
	
	if ( result.isEmpty() ) {
		pteLog->appendPlainText( tr( "You canceled the open file dialog" ) );
	}
	else {
		QStringList texts;
		
		texts << tr( "You accepted the open file dialog" );
		
		foreach ( const int& type, result.keys() ) {
			switch ( type ) {
				case pFileDialog::TextCodec:
					texts << tr( "TextCodec: %1" ).arg( result[ type ].toString() );
					break;
				case pFileDialog::OpenReadOnly:
					texts << tr( "OpenReadOnly: %1" ).arg( result[ type ].toString() );
					break;
				case pFileDialog::Directory:
					texts << tr( "Directory: %1" ).arg( result[ type ].toString() );
					break;
				case pFileDialog::FileName:
					texts << tr( "FileName: %1" ).arg( result[ type ].toString() );
					break;
				case pFileDialog::FileNames:
					texts << tr( "FileNames: %1" ).arg( result[ type ].toStringList().join( ", " ) );
					break;
				case pFileDialog::SelectedFilter:
					texts << tr( "SelectedFilter: %1" ).arg( result[ type ].toString() );
					break;
			}
		}
		
		pteLog->appendPlainText( texts.join( "\n" ) );
	}
}

void MainWindow::openDirectoryDialog()
{
	const QString caption;
	const QString dir;
	const QString filter;
	const bool enabledTextCodec = true;
	const bool enabledOpenReadOnly = true;
	const QString selectedFilter;
	const QFileDialog::Options options = 0;
	const pFileDialogResult result = pFileDialog::getExistingDirectory( this, caption, dir, enabledTextCodec,
		enabledOpenReadOnly, options | QFileDialog::ShowDirsOnly );
	
	pteLog->appendPlainText( QString::null );
	
	if ( result.isEmpty() ) {
		pteLog->appendPlainText( tr( "You canceled the open directory dialog" ) );
	}
	else {
		QStringList texts;
		
		texts << tr( "You accepted the open directory dialog" );
		
		foreach ( const int& type, result.keys() ) {
			switch ( type ) {
				case pFileDialog::TextCodec:
					texts << tr( "TextCodec: %1" ).arg( result[ type ].toString() );
					break;
				case pFileDialog::OpenReadOnly:
					texts << tr( "OpenReadOnly: %1" ).arg( result[ type ].toString() );
					break;
				case pFileDialog::Directory:
					texts << tr( "Directory: %1" ).arg( result[ type ].toString() );
					break;
				case pFileDialog::FileName:
					texts << tr( "FileName: %1" ).arg( result[ type ].toString() );
					break;
				case pFileDialog::FileNames:
					texts << tr( "FileNames: %1" ).arg( result[ type ].toStringList().join( ", " ) );
					break;
				case pFileDialog::SelectedFilter:
					texts << tr( "SelectedFilter: %1" ).arg( result[ type ].toString() );
					break;
			}
		}
		
		pteLog->appendPlainText( texts.join( "\n" ) );
	}
}
