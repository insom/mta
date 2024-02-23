mta
===

What is the stupidest mail transfer[^1] agent that could comply with [RFC5321][]? I hope it is this one.

`mta` requires you to customize the `HOMEDIR` and `EMAIL_ADDRESS` defines at the top, then `make` it.

It is launched from `inetd(8)` by adding this line to `/etc/inetd.conf`:

```
smtp stream tcp nowait insom /home/insom/Repo/mta/mta
```

(if you're not `insom` you should probably replace the username and path appropriately).

It will deliver email into a [Maildir][] that must already exist. If you don't have `maildirmake` installed you can get away with `mkdir -p Maildir/new Maildir/cur Maildir/tmp && chmod -R 0700 Maildir`.

I've experienced the real joy of having Fastmail deliver real email to my server and stored it in a Maildir where both `s-nail` and `neomutt` can read it correctly. Huzzah.

[^1]: It's almost more of a mail-delivery-agent, especially as it cannot handle outgoing email, but MDA is a specific term that means something else _not quite_ the same as this 🤷

[RFC5321]: https://www.rfc-editor.org/rfc/rfc5321.html
[Maildir]: https://cr.yp.to/proto/maildir.html
