<map version="0.7.1">
<node TEXT="BitFlood">
<node TEXT="peer protocol" POSITION="right">
<node TEXT="use cases">
<node ID="_" TEXT="initial connect">
<node TEXT="share piece sets"/>
</node>
<node TEXT="get piece">
<node TEXT="notify all"/>
</node>
<node TEXT="disconnect">
<node TEXT="clean"/>
<node TEXT="dirty (dropped)">
<node TEXT="handled by tracking last contact time for each peer"/>
</node>
</node>
</node>
<node TEXT="methods provided">
<node TEXT="exchangeChunkMap">
<edge WIDTH="thin"/>
<font NAME="SansSerif" SIZE="12"/>
<node TEXT="exchanges chunk maps between self and caller"/>
<node COLOR="#999999" TEXT="accepts: filenames, bitmaps of remote peer&apos;s chunks">
<font NAME="Default" SIZE="10"/>
</node>
<node COLOR="#999999" TEXT="returns: filenames, bitmaps of local peer&apos;s chunks">
<font NAME="Default" SIZE="10"/>
</node>
</node>
<node TEXT="getChunk">
<node TEXT="sends a chunk to the caller"/>
<node COLOR="#999999" TEXT="accepts: filename, chunk index">
<font NAME="Default" SIZE="10"/>
</node>
<node COLOR="#999999" TEXT="returns: chunk data">
<font NAME="Default" SIZE="10"/>
</node>
</node>
<node TEXT="disconnect">
<node TEXT="removes caller from peer list"/>
</node>
</node>
</node>
<node TEXT="network simulation" POSITION="right">
<node TEXT="links">
<node TEXT="research">
<node LINK="http://www.kom.e-technik.tu-darmstadt.de/publications/abstracts/BHMS04-1.html" TEXT="Analysis of Overlay Networks"/>
<node LINK="http://www.neurogrid.net/php/results.php" TEXT="NeuroGrid"/>
</node>
<node TEXT="simulation packages">
<node LINK="http://www.omnetpp.org/" TEXT="OMNet++"/>
<node LINK="http://www.isi.edu/nsnam/ns/" TEXT="NS2"/>
</node>
</node>
</node>
<node TEXT="open questions" POSITION="right">
<node TEXT="should chunks be transferred in a push or pull fashion?"/>
<node TEXT="should peers route some kind of information across multiple hops?"/>
<node TEXT="should the tracker try to make observations about the&#xa;state of the swarm and alter its behavior to help out?">
<node TEXT="does it even have enough information to do that?"/>
</node>
</node>
</node>
</map>
