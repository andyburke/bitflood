import java.util.Comparator;

/**
 * Created on Dec 3, 2004
 *
 */

/**
 * @author burke
 *
 */
public class ChunkPrioritizer implements Comparator
{

  public int compare(Object o1, Object o2) throws ClassCastException
  {
    if ( !(o1 instanceof Flood.RuntimeChunk) || !(o2 instanceof Flood.RuntimeChunk) )
    {
      throw new ClassCastException();
    }
    Flood.RuntimeChunk c1 = (Flood.RuntimeChunk) o1;
    Flood.RuntimeChunk c2 = (Flood.RuntimeChunk) o2;

    return c1.weight - c2.weight;
  }
  
}
