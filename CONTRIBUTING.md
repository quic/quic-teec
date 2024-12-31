## Contributing to QUIC-TEEC

Thank you for your interest in contributing to the QUIC-TEEC project! Your support is essential for keeping this project great and for making it better.

## Branching Strategy

In general, contributors should develop on branches based off of `main` and pull requests should be made against `main`.

## Contribution Checklist
Before raising a pull-request, please read and follow this checklist:
 
- It's a good idea to arrange a discussion with other developers to ensure there is consensus on large features, architecture changes, and other core code changes. PR reviews will go much faster when there are no surprises.
- Ensure that you follow the [Coding Style Guide](CODING-STYLE.md).
- Keep your change as focused as possible. Each commit should represent a logically separate change.
  If you want to make multiple independent changes, please consider submitting them as separate pull requests.
  In the description, provide context about what the changes do and why they should be made.
- Write a [good commit message](http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html).
- For every new feature, add Unit Tests.


## Submitting a pull request

1. Please read our [Code of Conduct](CODE-OF-CONDUCT.md) and [License](LICENSE.txt) before contributing.
1. [Fork](https://github.com/quic/quic-teec/fork) and clone the repository.
    
    ```bash
    git clone https://github.com/quic/quic-teec.git
    ``` 

1. Create a new branch based on `main`:

    ```bash 
    git checkout -b <my-branch-name> main
    ```

1. Create an upstream `remote` to make it easier to keep your branches up-to-date:

    ```bash
    git remote add upstream https://github.com/quic/quic-teec.git
    ```

1. Make your changes and ensuring you follow the [Coding Style Guide](CODING-STYLE.md).
1. Commit your changes using the [DCO](http://developercertificate.org/). You can attest to the DCO by commiting with the **-s** or **--signoff** options or manually adding the "Signed-off-by":
    
    ```bash
    git commit -s -m "Really useful commit message"`
    ```

1. After committing your changes on the topic branch, sync it with the upstream branch:

    ```bash
    git pull --rebase upstream main
    ```

1. Push to your fork.

    ```bash
    git push -u origin <my-branch-name>
    ```

    The `-u` is shorthand for `--set-upstream`. This will set up the tracking reference so subsequent runs of `git push` or `git pull` can omit the remote and branch.

1. [Submit a pull request](https://github.com/quic/<REPLACE-ME>/pulls) from your branch to `main`.
1. Pat yourself on the back and wait for your pull request to be reviewed.
1. Participate in the code review to catch issues early and ensure quality.
