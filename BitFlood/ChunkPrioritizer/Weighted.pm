package BitFlood::ChunkPrioritizer::Weighted;

use strict;
use base qw(BitFlood::ChunkPrioritizer);

use BitFlood::Debug;

use constant MAX_CHUNKS_TO_DOWNLOAD => 3;

sub FindChunk {
  my $self = shift;
  my $client = shift;
  my $flood = shift;
  
  return undef if($flood->{totalDownloading} >= MAX_CHUNKS_TO_DOWNLOAD);

  foreach my $neededChunk (@{$flood->neededChunksByWeight}) {
    my $file = $flood->Files->{$neededChunk->{filename}};
    my $chunk = $neededChunk->{chunk};

    foreach my $peer (@{$client->peers}) {
      next if !$peer->chunkMaps;
      my $peerChunkMap = $peer->chunkMaps->{$flood->contentHash}->{$file->{name}};
      next if !$peerChunkMap or !$peerChunkMap->bit_test($chunk->{index}) or $chunk->{downloading};
      return $peer, $file, $chunk;
    }
  }

  return undef;
}


1;
