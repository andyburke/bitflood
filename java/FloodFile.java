/*
 * Created on Nov 12, 2004
 *
 */

/**
 * @author burke
 *
 */
public class FloodFile 
{
  public class Chunk 
  {
    public String m_hash;
    public int    m_index;
    public int    m_size;
    public int    m_weight;
  }

  public class File 
  {
    public String  m_name;
    public int     m_size;
    public Chunk[] m_chunks;
  }

  public void ToXML( String o_xml )
  {
    o_xml = new String("");
  }

  public void FromXML( final String i_xml )
  {
  }
}
