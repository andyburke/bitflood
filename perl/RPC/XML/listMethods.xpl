<?xml version="1.0"?>
<!DOCTYPE methoddef SYSTEM "rpc-method.dtd">
<!--
    Generated automatically by make_method v1.09, Wed Aug  4 21:45:10 2004

    Any changes made here will be lost.
-->
<methoddef>
<name>system.listMethods</name>
<version>1.1</version>
<signature>array</signature>
<signature>array string</signature>
<help>
List all the methods known to the server. If the STRING parameter is passed,
it is used as a substring to match against, with only those matching methods
being returned. Note that the STRING parameter is not a regular expression,
but rather just a simple substring.
</help>
<code language="perl">
<![CDATA[
#!/usr/bin/perl
###############################################################################
#
#   Sub Name:       listMethods
#
#   Description:    Read the current list of methods from the server object
#                   and return the names in a list reference.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $srv      in      ref       Server object instance
#                   $pat      in      scalar    If passed, a substring to match
#                                                 names against. NOT a regex!
#
#   Globals:        None.
#
#   Environment:    None.
#
#   Returns:        listref
#
###############################################################################
sub listMethods
{
    use strict;

    my $srv = shift;
    my $pat = shift;

    my @list = sort $srv->list_methods;

    # Exclude any that are hidden from introspection APIs
    @list = grep(! $srv->get_method($_)->hidden, @list);
    @list = grep(index($_, $pat) != -1, @list) if ($pat);

    \@list;
}

__END__
]]></code>
</methoddef>
