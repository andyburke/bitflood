# NOTE: Derived from blib/lib/RPC/XML/Procedure.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Procedure;

#line 604 "blib/lib/RPC/XML/Procedure.pm (autosplit into blib/lib/auto/RPC/XML/Procedure/clone.al)"
###############################################################################
#
#   Sub Name:       clone
#
#   Description:    Create a near-exact copy of the invoking object, save that
#                   the listref in the "signature" key is a copy, not a ref
#                   to the same list.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $self     in      ref       Object of this class
#
#   Returns:        Success:    $new_self
#                   Failure:    error message
#
###############################################################################
sub clone
{
    my $self = shift;

    my $new_self = {};
    for (keys %$self)
    {
        next if $_ eq 'signature';
        $new_self->{$_} = $self->{$_};
    }
    $new_self->{signature} = [];
    @{$new_self->{signature}} = @{$self->{signature}};

    bless $new_self, ref($self);
}

# end of RPC::XML::Procedure::clone
1;
