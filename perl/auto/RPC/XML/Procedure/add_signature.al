# NOTE: Derived from blib/lib/RPC/XML/Procedure.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Procedure;

#line 658 "blib/lib/RPC/XML/Procedure.pm (autosplit into blib/lib/auto/RPC/XML/Procedure/add_signature.al)"
###############################################################################
#
#   Sub Name:       add_signature
#                   delete_signature
#
#   Description:    This pair of functions may be used to add and remove
#                   signatures from a method-object.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $self     in      ref       Object of this class
#                   @args     in      list      One or more signatures
#
#   Returns:        Success:    $self
#                   Failure:    error string
#
###############################################################################
sub add_signature
{
    my $self = shift;
    my @args = @_;

    my (%sigs, $one_sig, $tmp, $old);

    # Preserve the original in case adding the new one causes a problem
    $old = $self->{signature};
    %sigs = map { $_ => 1 } @{$self->{signature}};
    for $one_sig (@args)
    {
        $tmp = (ref $one_sig) ? join(' ', @$one_sig) : $one_sig;
        $sigs{$tmp} = 1;
    }
    $self->{signature} = [ keys %sigs ];
    unless (ref($tmp = $self->make_sig_table))
    {
        # Because this failed, we have to restore the old table and return
        # an error
        $self->{signature} = $old;
        $self->make_sig_table;
        return ref($self) . '::add_signature: Error re-hashing table: ' .
            $tmp;
    }

    $self;
}

# end of RPC::XML::Procedure::add_signature
1;
