package BitFlood::ChunkPrioritizer::Weighted;

use strict;
use base qw(BitFlood::ChunkPrioritizer);


sub FindChunk {
  my $self = shift;
  my $flood = shift;

  my %incompleteFiles;
  my $files = [ grep { ! $_->{chunkMap}->is_full } values %{$flood->Files} ];
  $incompleteFiles{$flood->contentHash} = $files if @$files;

  return undef if !%incompleteFiles;

  my ($bestFile, $bestChunk);
  while (my ($floodFileHash, $files) = each %incompleteFiles) {
    foreach my $file (@$files) {

      my $chunkMap = $file->{chunkMap};
      for (my $index = 0; $index < $chunkMap->Size; $index++) {
	next if $chunkMap->bit_test($index);
	foreach my $peer (@{$flood->peers}) {
	  my $peerChunkMap = $peer->chunkMaps->{$file->{name}};
	  next if !$peerChunkMap or !$peerChunkMap->bit_test($index);
#	  return ($file, $file->{Chunk}[$index]);
	  if(!defined($bestChunk) || $file->{Chunk}[$index]->{weight} > $bestChunk->{weight}) {
	      $bestFile = $file;
	      $bestChunk = $file->{Chunk}[$index];
	  }
	}
      }

    }
  }

  return $bestFile, $bestChunk;
}


1;
