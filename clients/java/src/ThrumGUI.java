/**
 * Created on Dec 5, 2004
 *
 */

import org.eclipse.swt.events.*;
import org.eclipse.swt.layout.*;
import org.eclipse.swt.widgets.*;
import org.eclipse.swt.SWT;

import javax.media.bean.playerbean.*;

/**
 * @author burke
 *
 */
public class ThrumGUI
{

  private Display display = null;
  private Shell shell     = null;
  
  public ThrumGUI()
  {
    display = new Display();
    shell = new Shell(display);
    shell.setText("Thrum Demo Client");
    shell.setSize(300, 300);
    shell.setLayout(new FillLayout());
    
  }
  
  public void InitializeComponents() {

    Menu menuBar = new Menu(shell, SWT.BAR);
    shell.setMenuBar(menuBar);
    
    MenuItem fileMenu = new MenuItem(menuBar, SWT.CASCADE);
    fileMenu.setText("File");
    
    Menu fileMenuItems = new Menu(fileMenu);
    fileMenu.setMenu(fileMenuItems);
    
    MenuItem openItem = new MenuItem(fileMenuItems, SWT.PUSH);
    openItem.setText("Open");
    openItem.addSelectionListener(
      new SelectionAdapter() {
        public void widgetSelected(SelectionEvent e) {
          OpenFile();
        }
      }
      														);

    MediaPlayer mediaPlayer = new MediaPlayer();
    
    
   
    

  }

  public void Display()
  {
    shell.pack();
    shell.open();
  }
  
  public void Loop()
  {
    while( !this.shell.isDisposed())
    {
      if(!this.display.readAndDispatch()) 
        this.display.sleep();
    }
  }

  public void CleanUp()
  {
    this.display.dispose();
  }
  
  public void OpenFile() {
    System.out.println("open...");
  }
  
}
