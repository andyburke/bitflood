# NOTE: Derived from blib/lib/RPC/XML/Function.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Function;

#line 236 "blib/lib/RPC/XML/Function.pm (autosplit into blib/lib/auto/RPC/XML/Function/clone.al)"
#
# These are the same as RPC::XML::Procedure subs, except that they have no
# references to signatures.
#
###############################################################################
#
#   Sub Name:       clone
#
#   Description:    Create a copy of the invoking object.
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
    %$new_self = %$self;

    bless $new_self, ref($self);
}

# end of RPC::XML::Function::clone
1;
