/**
 * Created on Dec 5, 2004
 *
 */


/**
 * @author burke
 *
 */
public class Thrum
{

	private org.eclipse.swt.widgets.Shell sShell = null;

  public static void main( String[] args )
  {
 
		/* Before this is run, be sure to set up the following in the launch configuration 
		 * (Arguments->VM Arguments) for the correct SWT library path. 
		 * The following is a windows example:
		 * -Djava.library.path="installation_directory\plugins\org.eclipse.swt.win32_3.0.0\os\win32\x86"
		 */
		org.eclipse.swt.widgets.Display display = org.eclipse.swt.widgets.Display.getDefault();		
		Thrum thisClass = new Thrum();
		thisClass.createSShell() ;
		thisClass.sShell.open();
		while (!thisClass.sShell.isDisposed()) {
			if (!display.readAndDispatch()) display.sleep ();
		}
		display.dispose();		
 }
	/**
	 * This method initializes sShell
	 */
	private void createSShell() {
		sShell = new org.eclipse.swt.widgets.Shell();		   
		sShell.setSize(new org.eclipse.swt.graphics.Point(300,200));
		sShell.setText("Shell");
	}
}
