/**
 * Created on Dec 5, 2004
 *
 */

/**
 * @author burke
 *
 */
public class ThrumDemo
{

  public static void main( String[] args )
  {
    ThrumHTTPProxy t = new ThrumHTTPProxy(args[0]);
    
    t.Initialize();
    
    while(true)
    {
      t.LoopOnce();
      try
      {
        Thread.sleep(100);
      }
      catch(Exception e)
      {
        e.printStackTrace();
      }
    }
    /*
    ThrumGUI window = new ThrumGUI();
    window.InitializeComponents();
    window.Display();
    window.Loop();
    window.CleanUp();
    */
  }
}
