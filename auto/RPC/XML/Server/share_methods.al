# NOTE: Derived from blib/lib/RPC/XML/Server.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Server;

#line 1855 "blib/lib/RPC/XML/Server.pm (autosplit into blib/lib/auto/RPC/XML/Server/share_methods.al)"
sub share_methods
{
    my $self    = shift;
    my $src_srv = shift;
    my @names   = @_;

    my ($me, $pkg, %tmp, @tmp, $tmp, $meth, @list, @missing);

    $me = ref($self) . '::share_methods';
    $pkg = __PACKAGE__; # So it can go inside quoted strings

    return "$me: First arg not derived from $pkg, cannot share"
        unless ((ref $src_srv) && (UNIVERSAL::isa($src_srv, $pkg)));
    return "$me: Must specify at least one method name for sharing"
        unless @names;

    #
    # Scan @names for any regez objects, and if found insert the matches into
    # the list.
    #
    # Only do this once:
    #
    @tmp = keys %{$src_srv->{__method_table}};
    for $tmp (@names)
    {
        if (ref($names[$tmp]) eq 'Regexp')
        {
            $tmp{$_}++ for (grep($_ =~ $tmp, @tmp));
        }
        else
        {
            $tmp{$tmp}++;
        }
    }
    # This has the benefit of trimming any redundancies caused by regex's
    @names = keys %tmp;

    #
    # Note that the method refs are saved until we've verified all of them.
    # If we have to return a failure message, I don't want to leave a half-
    # finished job or have to go back and undo (n-1) additions because of one
    # failure.
    #
    for (@names)
    {
        $meth = $src_srv->get_method($_);
        if (ref $meth)
        {
            push(@list, $meth);
        }
        else
        {
            push(@missing, $_);
        }
    }

    if (@missing)
    {
        return "$me: One or more methods not found on source object: @missing";
    }
    else
    {
        $self->add_method($_) for (@list);
    }

    $self;
}

# end of RPC::XML::Server::share_methods
1;
