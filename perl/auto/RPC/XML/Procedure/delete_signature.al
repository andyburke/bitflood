# NOTE: Derived from blib/lib/RPC/XML/Procedure.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Procedure;

#line 703 "blib/lib/RPC/XML/Procedure.pm (autosplit into blib/lib/auto/RPC/XML/Procedure/delete_signature.al)"
sub delete_signature
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
        delete $sigs{$tmp};
    }
    $self->{signature} = [ keys %sigs ];
    unless (ref($tmp = $self->make_sig_table))
    {
        # Because this failed, we have to restore the old table and return
        # an error
        $self->{signature} = $old;
        $self->make_sig_table;
        return ref($self) . '::delete_signature: Error re-hashing table: ' .
            $tmp;
    }

    $self;
}

# end of RPC::XML::Procedure::delete_signature
1;
