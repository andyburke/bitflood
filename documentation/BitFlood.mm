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
<node COLOR="#999999" TEXT="accepts: bit vector; map of remote peer&apos;s chunks">
<font NAME="Default" SIZE="10"/>
</node>
<node COLOR="#999999" TEXT="returns: bit vector; map of local peer&apos;s chunks">
<font NAME="Default" SIZE="10"/>
</node>
</node>
<node TEXT="getChunk">
<node TEXT="sends a chunk to the caller"/>
<node COLOR="#999999" TEXT="accepts: chunk index (optional file name, if chunk indexes not global)">
<font NAME="Default" SIZE="10"/>
</node>
<node COLOR="#999999" TEXT="returns: data; chunk data">
<font NAME="Default" SIZE="10"/>
</node>
</node>
<node TEXT="disconnect">
<node TEXT="removes caller from peer list"/>
</node>
</node>
</node>
</node>
</map>
