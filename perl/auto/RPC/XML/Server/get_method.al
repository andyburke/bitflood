# NOTE: Derived from blib/lib/RPC/XML/Server.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Server;

#line 1142 "blib/lib/RPC/XML/Server.pm (autosplit into blib/lib/auto/RPC/XML/Server/get_method.al)"
sub get_method
{
    my $self = shift;
    my $name = shift;

    my $meth = $self->{__method_table}->{$name};
    unless (defined $meth)
    {
        if ($self->{__auto_methods})
        {
            # Try to load this dynamically on the fly, from any of the dirs
            # that are in this object's @xpl_path
            (my $loadname = $name) =~ s/^system\.//;
            $self->add_method("$loadname.xpl");
        }
        # If method is still not in the table, we were unable to load it
        return "Unknown method: $name"
            unless $meth = $self->{__method_table}->{$name};
    }
    # Check the mod-time of the file the method came from, if the test is on
    if ($self->{__auto_updates} && $meth->{file} &&
        ($meth->{mtime} < (stat $meth->{file})[9]))
    {
        my $ret = $meth->reload;
        return "Reload of method $name failed: $ret" unless ref($ret);
    }

    $meth;
}

# end of RPC::XML::Server::get_method
1;
